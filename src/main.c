#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"
#include "utils.h"
#include "workout.h"

/* Static string copy helper for standard ISO C compliance */
static char* str_dup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, str, len);
    return dup;
}

int main(int argc, char *argv[]) {

    /* Route: soma workout log --exercise ... --sets ... --reps ... --weight ... */
    if (argc >= 3 && strcmp(argv[1], "workout") == 0 && strcmp(argv[2], "log") == 0) {
        fprintf(stderr, "DEBUG: Parsing workout arguments (argc=%d)\n", argc - 2);
        /* Parse workout arguments starting from argv[3] */
        Workout* w = parse_workout_args(argc - 2, argv + 2);
        
        fprintf(stderr, "DEBUG: Parsed workout, checking errors\n");
        if (!w) {
            fprintf(stderr, "error: failed to parse workout arguments\n");
            return 1;
        }
        
        /* Check for parsing errors */
        if (w->error != WORKOUT_OK) {
            fprintf(stderr, "error: %s\n", w->error_msg ? w->error_msg : "unknown error");
            free_workout(w);
            return 1;
        }
        
        fprintf(stderr, "DEBUG: Logging workout to database\n");
        /* Log the workout to database */
        if (!log_workout(w)) {
            fprintf(stderr, "error: failed to log workout to database\n");
            free_workout(w);
            return 1;
        }
        
        fprintf(stderr, "DEBUG: Workout logged successfully\n");
        /* Print success confirmation */
        printf(POSITIVE "✓ " RESET "Logged " ACCENT BOLD "%s" RESET ": %d sets, " 
               ACCENT "%.0f kg-reps" RESET "\n", 
               w->exercise, w->num_sets, w->volume_kg);
        
        /* Print PR badge if new personal record */
        if (w->is_new_pr) {
            double max_weight = get_max_weight(w->weights);
            if (max_weight > 0) {
                printf("\n");
                print_pr_badge(w->exercise, max_weight);
            }
        }
        
        free_workout(w);
        return 0;
    }
    
    /* Default: show demo/visualization (existing code) */
    sqlite3 *db = db_open();
    
    /* Seed the database if it is empty to ensure a beautiful initial experience */
    db_seed(db);

    /* ── soma today ── */
    print_logo();
    print_separator();

    int days_since = 0;
    char last_day[64] = "never trained";
    db_get_last_trained(db, &days_since, last_day, sizeof(last_day));

    int sleep_logged = 0;
    double sleep_hrs = 0.0;
    sqlite3_stmt *stmt_sleep;
    if (sqlite3_prepare_v2(db, "SELECT hours FROM sleep_log WHERE logged_at >= datetime('now', 'start of day', 'localtime') LIMIT 1;", -1, &stmt_sleep, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt_sleep) == SQLITE_ROW) {
            sleep_hrs = sqlite3_column_double(stmt_sleep, 0);
            sleep_logged = 1;
        }
        sqlite3_finalize(stmt_sleep);
    }

    int weight_logged = 0;
    sqlite3_stmt *stmt_wt;
    if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM bodyweight WHERE logged_at >= datetime('now', 'start of day', 'localtime');", -1, &stmt_wt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt_wt) == SQLITE_ROW) {
            if (sqlite3_column_int(stmt_wt, 0) > 0) {
                weight_logged = 1;
            }
        }
        sqlite3_finalize(stmt_wt);
    }

    print_today_header(days_since, last_day, sleep_hrs, sleep_logged, weight_logged);

    /* Suggest next training target dynamically based on the last workout logged */
    printf(ACCENT "  ❯ " RESET BOLD WHITE);
    if (strstr(last_day, "bench") || strstr(last_day, "press")) {
        printf("legs day — attempt squat 122.5kg × 5 today");
    } else if (strstr(last_day, "squat")) {
        printf("pull day — focus on back density and deadlifts today");
    } else {
        printf("push day — attempt bench 82.5kg × 5 today");
    }
    printf(RESET "\n\n");

    print_separator();

    /* ── soma status ── */
    print_logo();
    print_separator();

    print_section("training");
    printf(DIM "  %-20s%-14s%-16s%s\n\n" RESET,
           "exercise", "last set", "trend", "est. 1rm");

    /* Bench Press */
    double bench_hist[6];
    int bench_count = db_get_exercise_history(db, "bench press", bench_hist, NULL, NULL, 6);
    double bench_last_w = 0.0;
    int bench_last_r = 0;
    db_get_latest_workout_set(db, "bench press", &bench_last_w, &bench_last_r);
    double bench_1rm = db_get_estimated_1rm(db, "bench press");
    char bench_set_str[32] = "—";
    if (bench_last_w > 0) snprintf(bench_set_str, sizeof(bench_set_str), "%.0fkg × %d", bench_last_w, bench_last_r);

    printf(DIM "  %-20s" RESET, "bench press");
    printf(ACCENT BOLD "%-14s" RESET, bench_set_str);
    print_sparkline(bench_hist, bench_count);
    if (bench_1rm > 0) printf(POSITIVE "  %.1fkg\n\n" RESET, bench_1rm);
    else printf(DIM "  —\n\n" RESET);

    /* Squat */
    double squat_hist[6];
    int squat_count = db_get_exercise_history(db, "squat", squat_hist, NULL, NULL, 6);
    double squat_last_w = 0.0;
    int squat_last_r = 0;
    db_get_latest_workout_set(db, "squat", &squat_last_w, &squat_last_r);
    double squat_1rm = db_get_estimated_1rm(db, "squat");
    char squat_set_str[32] = "—";
    if (squat_last_w > 0) snprintf(squat_set_str, sizeof(squat_set_str), "%.0fkg × %d", squat_last_w, squat_last_r);

    printf(DIM "  %-20s" RESET, "squat");
    printf(ACCENT BOLD "%-14s" RESET, squat_set_str);
    print_sparkline(squat_hist, squat_count);
    if (squat_1rm > 0) printf(POSITIVE "  %.1fkg\n\n" RESET, squat_1rm);
    else printf(DIM "  —\n\n" RESET);

    /* Overhead Press */
    double ohp_hist[6];
    int ohp_count = db_get_exercise_history(db, "overhead press", ohp_hist, NULL, NULL, 6);
    double ohp_last_w = 0.0;
    int ohp_last_r = 0;
    db_get_latest_workout_set(db, "overhead press", &ohp_last_w, &ohp_last_r);
    double ohp_1rm = db_get_estimated_1rm(db, "overhead press");
    char ohp_set_str[32] = "—";
    if (ohp_last_w > 0) snprintf(ohp_set_str, sizeof(ohp_set_str), "%.0fkg × %d", ohp_last_w, ohp_last_r);

    printf(DIM "  %-20s" RESET, "overhead press");
    printf(ACCENT BOLD "%-14s" RESET, ohp_set_str);
    print_sparkline(ohp_hist, ohp_count);
    if (ohp_1rm > 0) printf(POSITIVE "  %.1fkg\n" RESET, ohp_1rm);
    else printf(DIM "  —\n" RESET);

    /* Physique Section */
    print_section("physique");

    double wt_hist[6];
    double wst_hist[6];
    int wt_count = db_get_bodyweight_history(db, wt_hist, wst_hist, 6);

    double latest_wt = wt_count > 0 ? wt_hist[wt_count - 1] : 0.0;
    double latest_wst = wt_count > 0 ? wst_hist[wt_count - 1] : 0.0;
    
    double wt_change = db_get_bodyweight_weekly_change(db);
    double wst_change = db_get_waist_weekly_change(db);

    char wt_label[32] = "—";
    if (latest_wt > 0) snprintf(wt_label, sizeof(wt_label), "%.1fkg", latest_wt);
    char wst_label[32] = "—";
    if (latest_wst > 0) snprintf(wst_label, sizeof(wst_label), "%.0fcm", latest_wst);

    char wt_change_str[32] = "";
    if (wt_change != 0.0) snprintf(wt_change_str, sizeof(wt_change_str), "%+.1fkg/wk", wt_change);
    char wst_change_str[32] = "";
    if (wst_change != 0.0) snprintf(wst_change_str, sizeof(wst_change_str), "%+.0fcm/wk", wst_change);

    printf(DIM "  %-20s" RESET, "bodyweight");
    printf(ACCENT BOLD "%-14s" RESET, wt_label);
    print_sparkline(wt_hist, wt_count);
    if (wt_count >= 2) printf(wt_change <= 0 ? POSITIVE "  %s\n\n" RESET : WARNING "  %s\n\n" RESET, wt_change_str);
    else printf(DIM "  —\n\n" RESET);

    printf(DIM "  %-20s" RESET, "waist");
    printf(ACCENT BOLD "%-14s" RESET, wst_label);
    print_sparkline(wst_hist, wt_count);
    if (wt_count >= 2 && latest_wst > 0) printf(wst_change <= 0 ? POSITIVE "  %s\n" RESET : WARNING "  %s\n" RESET, wst_change_str);
    else printf(DIM "  —\n" RESET);

    /* Recovery Section */
    print_section("recovery");

    double sl_hist[6];
    int sl_count = db_get_sleep_history(db, sl_hist, 6);
    double sleep_avg = db_get_sleep_7d_avg(db);
    char sleep_avg_str[32] = "—";
    if (sleep_avg > 0) snprintf(sleep_avg_str, sizeof(sleep_avg_str), "%.1fh", sleep_avg);

    printf(DIM "  %-20s" RESET, "sleep 7d avg");
    printf(ACCENT BOLD "%-14s" RESET, sleep_avg_str);
    print_sparkline(sl_hist, sl_count);
    printf(DIM "  target 7.5h\n\n" RESET);

    printf(DIM "  %-20s" RESET, "readiness");
    double readiness_score = (sleep_avg > 0) ? (sleep_avg * 10.0) : 70.0;
    if (readiness_score > 100.0) readiness_score = 100.0;
    char readiness_str[32];
    snprintf(readiness_str, sizeof(readiness_str), "%.0f / 100", readiness_score);
    printf(ACCENT BOLD "%-14s" RESET, readiness_str);
    if (readiness_score >= 75.0) {
        printf(POSITIVE "excellent - ready to crush it\n" RESET);
    } else if (readiness_score >= 65.0) {
        printf(POSITIVE "good to train\n" RESET);
    } else {
        printf(WARNING "fatigued - consider active recovery\n" RESET);
    }

    /* Nutrition Section */
    print_section("nutrition");
    double nut_cal = 0, nut_prot = 0, nut_carb = 0, nut_fat = 0;
    db_get_today_nutrition(db, &nut_cal, &nut_prot, &nut_carb, &nut_fat);

    print_bar("protein",  nut_prot, 200.0, "g");
    print_bar("calories", nut_cal,  2800.0, "kcal");
    print_bar("carbs",    nut_carb, 300.0, "g");
    print_bar("fat",       nut_fat,  70.0, "g");

    if (nut_prot < 150.0) {
        print_verdict("progressing well — macros are low today, focus on hitting protein targets!");
    } else {
        print_verdict("progressing well — recovery is fully optimized. attempt target weights next session!");
    }

    /* ── soma bench 80x5 CLI Log Mock ── */
    printf(DIM "  $ soma bench 80x5\n\n" RESET);
    print_separator();
    printf("\n");
    printf(ACCENT BOLD "  bench press\n\n" RESET);

    /* Get the two most recent bench logs to show delta progress */
    sqlite3_stmt *stmt_bench;
    double latest_b_w = 80.0, prev_b_w = 77.5;
    int latest_b_r = 5, prev_b_r = 5;
    double latest_b_vol = 400.0, prev_b_vol = 387.5;

    const char *sql_bench = "SELECT weight_kg, reps, volume_kg FROM workouts WHERE LOWER(exercise) = 'bench press' ORDER BY logged_at DESC, id DESC LIMIT 2;";
    if (sqlite3_prepare_v2(db, sql_bench, -1, &stmt_bench, NULL) == SQLITE_OK) {
        int idx = 0;
        while (sqlite3_step(stmt_bench) == SQLITE_ROW) {
            const char *w_str = (const char *)sqlite3_column_text(stmt_bench, 0);
            const char *r_str = (const char *)sqlite3_column_text(stmt_bench, 1);
            double vol = sqlite3_column_double(stmt_bench, 2);

            double w_val = 0;
            if (w_str) {
                char *dup = str_dup(w_str);
                char *tok = strtok(dup, ",");
                while (tok) { w_val = atof(tok); tok = strtok(NULL, ","); }
                free(dup);
            }
            int r_val = 0;
            if (r_str) {
                char *dup = str_dup(r_str);
                char *tok = strtok(dup, ",");
                while (tok) { r_val = atoi(tok); tok = strtok(NULL, ","); }
                free(dup);
            }

            if (idx == 0) {
                latest_b_w = w_val; latest_b_r = r_val; latest_b_vol = vol;
            } else {
                prev_b_w = w_val; prev_b_r = r_val; prev_b_vol = vol;
            }
            idx++;
        }
        sqlite3_finalize(stmt_bench);
    }

    double latest_b_1rm = latest_b_w * (1.0 + latest_b_r / 30.0);
    double prev_b_1rm = prev_b_w * (1.0 + prev_b_r / 30.0);
    double w_delta = latest_b_w - prev_b_w;
    double orm_delta = latest_b_1rm - prev_b_1rm;

    char log_w_str[32], log_prev_str[32], log_1rm_str[32], log_prev_1rm_str[32], log_vol_str[32];
    snprintf(log_w_str, sizeof(log_w_str), "%.0fkg × %d", latest_b_w, latest_b_r);
    snprintf(log_prev_str, sizeof(log_prev_str), "%.0fkg × %d", prev_b_w, prev_b_r);
    snprintf(log_1rm_str, sizeof(log_1rm_str), "%.1fkg", latest_b_1rm);
    snprintf(log_prev_1rm_str, sizeof(log_prev_1rm_str), "%+.1fkg", orm_delta);

    snprintf(log_vol_str, sizeof(log_vol_str), "%.0fkg", latest_b_vol);

    char w_delta_str[32] = "";
    if (w_delta > 0) snprintf(w_delta_str, sizeof(w_delta_str), "+%.1fkg", w_delta);
    else if (w_delta < 0) snprintf(w_delta_str, sizeof(w_delta_str), "%.1fkg", w_delta);

    print_log_row("logged",      log_w_str,      "");
    print_log_row("last",        log_prev_str,   w_delta_str);
    print_log_row("est. 1rm",    log_1rm_str,    log_prev_1rm_str);
    print_log_row("volume",      log_vol_str,    "");
    print_log_row("session vol", log_vol_str,    "");
    print_pr_badge("bench press", latest_b_1rm);

    /* ── soma last bench ── */
    printf(DIM "  $ soma last bench\n\n" RESET);
    print_separator();
    printf("\n");
    printf(ACCENT BOLD "  bench press" RESET
           DIM " — last 5 sessions\n\n" RESET);

    /* Fetch 5 history sessions for bench press dynamically */
    sqlite3_stmt *stmt_hist;
    const char *sql_hist = 
        "SELECT logged_at, weight_kg, reps, volume_kg FROM workouts "
        "WHERE LOWER(exercise) = 'bench press' "
        "ORDER BY logged_at DESC, id DESC LIMIT 5;";

    double hist_vols[5] = {0};
    int hist_count = 0;

    if (sqlite3_prepare_v2(db, sql_hist, -1, &stmt_hist, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt_hist) == SQLITE_ROW && hist_count < 5) {
            const char *date_str = (const char *)sqlite3_column_text(stmt_hist, 0);
            const char *w_str = (const char *)sqlite3_column_text(stmt_hist, 1);
            const char *r_str = (const char *)sqlite3_column_text(stmt_hist, 2);
            double vol = sqlite3_column_double(stmt_hist, 3);

            hist_vols[hist_count] = vol;

            /* Parse date for display (convert YYYY-MM-DD HH:MM:SS -> e.g. "may 14") */
            char display_date[32] = "";
            if (date_str) {
                int year = 0, month = 0, day = 0;
                if (sscanf(date_str, "%d-%d-%d", &year, &month, &day) == 3) {
                    const char *months_names[] = {
                        "jan", "feb", "mar", "apr", "may", "jun",
                        "jul", "aug", "sep", "oct", "nov", "dec"
                    };
                    if (month >= 1 && month <= 12) {
                        snprintf(display_date, sizeof(display_date), "%s %02d", months_names[month - 1], day);
                    }
                }
            }
            if (display_date[0] == '\0') {
                snprintf(display_date, sizeof(display_date), "session %d", hist_count + 1);
            }

            char hist_val_str[64];
            /* extract peak or last set */
            double w_val = 0;
            if (w_str) {
                char *dup = str_dup(w_str); char *tok = strtok(dup, ",");
                while (tok) { w_val = atof(tok); tok = strtok(NULL, ","); }
                free(dup);
            }
            int r_val = 0;
            if (r_str) {
                char *dup = str_dup(r_str); char *tok = strtok(dup, ",");
                while (tok) { r_val = atoi(tok); tok = strtok(NULL, ","); }
                free(dup);
            }


            snprintf(hist_val_str, sizeof(hist_val_str), "%.0fkg × %d", w_val, r_val);
            char hist_vol_str[32];
            snprintf(hist_vol_str, sizeof(hist_vol_str), "%.0fkg", vol);

            print_hist_row(display_date, hist_val_str, hist_vol_str, (hist_count == 0));
            hist_count++;
        }
        sqlite3_finalize(stmt_hist);
    }

    printf("\n");

    /* Reverse volumes order to feed left-to-right into print_volume_trend */
    double reversed_vols[5];
    for (int i = 0; i < hist_count; i++) {
        reversed_vols[i] = hist_vols[hist_count - 1 - i];
    }
    
    /* Calculate percent volume change between earliest and latest */
    double pct_change = 0.0;
    if (hist_count >= 2 && reversed_vols[0] > 0) {
        pct_change = ((reversed_vols[hist_count - 1] - reversed_vols[0]) / reversed_vols[0]) * 100.0;
    }

    print_volume_trend(reversed_vols, hist_count, pct_change);

    /* Dynamic targets */
    double next_target_w = latest_b_w;
    if (hist_count >= 2 && pct_change >= 0) {
        next_target_w += 2.5;
    }
    printf(DIM "  %-20s" RESET ACCENT "%+.1fkg / week\n" RESET, "trend", (hist_count >= 2) ? (latest_b_w - reversed_vols[0]) / 4.0 : 1.5);
    printf(DIM "  %-20s" RESET ACCENT BOLD "%.1fkg × %d" RESET DIM "  next session\n\n" RESET, "target", next_target_w, latest_b_r);

    db_close(db);
    return 0;
}
