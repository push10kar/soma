#include "status.h"
#include "db.h"
#include "utils.h"
#include "workout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

/* Local ISO C compliance helper to duplicate strings */
static char *local_str_dup(const char *str) {
  if (!str)
    return NULL;
  size_t len = strlen(str) + 1;
  char *dup = (char *)malloc(len);
  if (dup)
    memcpy(dup, str, len);
  return dup;
}

/* Portable, warning-free case-insensitive substring matcher */
static bool contains_string(const char *haystack, const char *needle) {
  if (!haystack || !needle) return false;
  char hay_low[128];
  char ndl_low[128];

  int i;
  for (i = 0; haystack[i] != '\0' && i < 127; i++) {
    char c = haystack[i];
    if (c >= 'A' && c <= 'Z') c += 32;
    hay_low[i] = c;
  }
  hay_low[i] = '\0';

  for (i = 0; needle[i] != '\0' && i < 127; i++) {
    char c = needle[i];
    if (c >= 'A' && c <= 'Z') c += 32;
    ndl_low[i] = c;
  }
  ndl_low[i] = '\0';

  return strstr(hay_low, ndl_low) != NULL;
}

void print_today_briefing(sqlite3 *db) {
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
  if (contains_string(last_day, "bench") || contains_string(last_day, "press")) {
    printf("legs day — attempt squat 122.5kg × 5 today");
  } else if (contains_string(last_day, "squat")) {
    printf("pull day — focus on back density and deadlifts today");
  } else {
    printf("push day — attempt bench 82.5kg × 5 today");
  }
  printf(RESET "\n\n");

  print_separator();
}

void print_physique_dashboard(sqlite3 *db) {
  print_logo();
  print_separator();

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

  print_separator();
}

void print_recovery_dashboard(sqlite3 *db) {
  print_logo();
  print_separator();

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

  print_separator();
}

void print_nutrition_dashboard(sqlite3 *db) {
  print_logo();
  print_separator();

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

  print_separator();
}

void print_training_dashboard(sqlite3 *db) {
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

  print_separator();
}

void print_full_status(sqlite3 *db) {
  // Prints all sections in a combined layout
  print_logo();
  print_separator();

  // 1. Training Dashboard (extracted from print_training_dashboard without redundant logos)
  print_section("training");
  printf(DIM "  %-20s%-14s%-16s%s\n\n" RESET,
         "exercise", "last set", "trend", "est. 1rm");

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

  // 2. Physique
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

  // 3. Recovery
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

  // 4. Nutrition
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

  print_separator();
}

void print_exercise_history(sqlite3 *db, const char *input_ex) {
  // Identify which exercise matches the input
  const char *ex_name = NULL;
  if (strcasecmp(input_ex, "bench") == 0 || strcasecmp(input_ex, "bench press") == 0) {
    ex_name = "bench press";
  } else if (strcasecmp(input_ex, "squat") == 0) {
    ex_name = "squat";
  } else if (strcasecmp(input_ex, "ohp") == 0 || strcasecmp(input_ex, "overhead") == 0 || strcasecmp(input_ex, "overhead press") == 0) {
    ex_name = "overhead press";
  } else {
    // Fallback to substring matching
    if (contains_string("bench press", input_ex)) ex_name = "bench press";
    else if (contains_string("squat", input_ex)) ex_name = "squat";
    else if (contains_string("overhead press", input_ex) || contains_string("ohp", input_ex)) ex_name = "overhead press";
  }

  if (!ex_name) {
    fprintf(stderr, "error: exercise '%s' not recognized. Choose bench, squat, or ohp.\n", input_ex);
    return;
  }

  print_logo();
  print_separator();
  printf("\n");
  printf(ACCENT BOLD "  %s" RESET DIM " — last 5 sessions\n\n" RESET, ex_name);

  sqlite3_stmt *stmt_hist;
  const char *sql_hist =
      "SELECT logged_at, weight_kg, reps, volume_kg FROM workouts "
      "WHERE LOWER(exercise) = ? "
      "ORDER BY logged_at DESC, id DESC LIMIT 5;";

  double hist_vols[5] = {0};
  int hist_count = 0;
  double latest_w = 0.0;
  int latest_r = 0;
  double latest_vol = 0.0;
  double oldest_vol = 0.0;
  double oldest_w = 0.0;

  if (sqlite3_prepare_v2(db, sql_hist, -1, &stmt_hist, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt_hist, 1, ex_name, -1, SQLITE_STATIC);
    while (sqlite3_step(stmt_hist) == SQLITE_ROW && hist_count < 5) {
      const char *date_str = (const char *)sqlite3_column_text(stmt_hist, 0);
      const char *w_str = (const char *)sqlite3_column_text(stmt_hist, 1);
      const char *r_str = (const char *)sqlite3_column_text(stmt_hist, 2);
      double vol = sqlite3_column_double(stmt_hist, 3);

      hist_vols[hist_count] = vol;

      // Parse date for display
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

      // Extract weights and reps
      double w_val = 0;
      if (w_str) {
        char *dup = local_str_dup(w_str);
        char *tok = strtok(dup, ",");
        while (tok) {
          w_val = atof(tok);
          tok = strtok(NULL, ",");
        }
        free(dup);
      }
      int r_val = 0;
      if (r_str) {
        char *dup = local_str_dup(r_str);
        char *tok = strtok(dup, ",");
        while (tok) {
          r_val = atoi(tok);
          tok = strtok(NULL, ",");
        }
        free(dup);
      }

      if (hist_count == 0) {
        latest_w = w_val;
        latest_r = r_val;
        latest_vol = vol;
      }
      oldest_w = w_val;

      char hist_val_str[64];
      snprintf(hist_val_str, sizeof(hist_val_str), "%.0fkg × %d", w_val, r_val);
      char hist_vol_str[32];
      snprintf(hist_vol_str, sizeof(hist_vol_str), "%.0fkg", vol);

      print_hist_row(display_date, hist_val_str, hist_vol_str, (hist_count == 0));
      hist_count++;
    }
    sqlite3_finalize(stmt_hist);
  }

  if (hist_count == 0) {
    printf(DIM "  No historical workouts found for %s.\n\n" RESET, ex_name);
    print_separator();
    return;
  }

  printf("\n");

  // Reverse volumes order to feed left-to-right into print_volume_trend
  double reversed_vols[5];
  for (int i = 0; i < hist_count; i++) {
    reversed_vols[i] = hist_vols[hist_count - 1 - i];
  }
  oldest_vol = reversed_vols[0];

  double pct_change = 0.0;
  if (hist_count >= 2 && oldest_vol > 0) {
    pct_change = ((latest_vol - oldest_vol) / oldest_vol) * 100.0;
  }

  print_volume_trend(reversed_vols, hist_count, pct_change);

  double next_target_w = latest_w;
  if (hist_count >= 2 && pct_change >= 0) {
    next_target_w += (strcmp(ex_name, "overhead press") == 0) ? 1.0 : 2.5;
  }

  double weekly_trend = (hist_count >= 2) ? (latest_w - oldest_w) / 4.0 : 1.5;
  printf(DIM "  %-20s" RESET ACCENT "%+.1fkg / week\n" RESET, "trend", weekly_trend);
  printf(DIM "  %-20s" RESET ACCENT BOLD "%.1fkg × %d" RESET DIM "  next session\n\n" RESET, "target", next_target_w, latest_r);

  print_separator();
}
