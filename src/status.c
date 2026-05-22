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


static double get_prev_max_weight(sqlite3 *db, const char *exercise) {
  sqlite3_stmt *stmt;
  const char *sql = "SELECT weight_kg FROM workouts WHERE LOWER(exercise) = LOWER(?) AND logged_at < datetime('now', '-7 days', 'localtime');";
  double prev_max = 0.0;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, exercise, -1, SQLITE_STATIC);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      const char *w_str = (const char *)sqlite3_column_text(stmt, 0);
      if (w_str) {
        char *dup = local_str_dup(w_str);
        char *tok = strtok(dup, ",");
        while (tok) {
          double w = atof(tok);
          if (w > prev_max) {
            prev_max = w;
          }
          tok = strtok(NULL, ",");
        }
        free(dup);
      }
    }
    sqlite3_finalize(stmt);
  }
  return prev_max;
}

static void render_progress_bar(char *out_bar, size_t max_len, double current, double target, int width) {
  int filled = 0;
  if (target > 0.0) {
    double pct = current / target;
    if (pct > 1.0) pct = 1.0;
    if (pct < 0.0) pct = 0.0;
    filled = (int)(pct * width);
  }
  char *p = out_bar;
  for (int i = 0; i < width && (p - out_bar) < (int)max_len - 4; i++) {
    if (i < filled) {
      strcpy(p, "█");
      p += 3;
    } else {
      strcpy(p, "░");
      p += 3;
    }
  }
  *p = '\0';
}

void print_weekly_review(sqlite3 *db) {
  char start_date_raw[32] = "";
  char end_date_raw[32] = "";

  sqlite3_stmt *stmt_dates;
  const char *sql_dates = "SELECT date('now', '-7 days', 'localtime'), date('now', 'localtime');";
  if (sqlite3_prepare_v2(db, sql_dates, -1, &stmt_dates, NULL) == SQLITE_OK) {
    if (sqlite3_step(stmt_dates) == SQLITE_ROW) {
      const char *s = (const char *)sqlite3_column_text(stmt_dates, 0);
      const char *e = (const char *)sqlite3_column_text(stmt_dates, 1);
      if (s) strcpy(start_date_raw, s);
      if (e) strcpy(end_date_raw, e);
    }
    sqlite3_finalize(stmt_dates);
  }

  char week_range[128] = "N/A";
  int s_yr = 0, s_mo = 0, s_dy = 0;
  int e_yr = 0, e_mo = 0, e_dy = 0;
  
  const char *months_names[] = {
      "jan", "feb", "mar", "apr", "may", "jun",
      "jul", "aug", "sep", "oct", "nov", "dec"
  };

  if (sscanf(start_date_raw, "%d-%d-%d", &s_yr, &s_mo, &s_dy) == 3 &&
      sscanf(end_date_raw, "%d-%d-%d", &e_yr, &e_mo, &e_dy) == 3) {
    const char *s_month = (s_mo >= 1 && s_mo <= 12) ? months_names[s_mo - 1] : "unk";
    const char *e_month = (e_mo >= 1 && e_mo <= 12) ? months_names[e_mo - 1] : "unk";

    if (strcmp(s_month, e_month) == 0) {
      snprintf(week_range, sizeof(week_range), "%s %d – %d", s_month, s_dy, e_dy);
    } else {
      snprintf(week_range, sizeof(week_range), "%s %d – %s %d", s_month, s_dy, e_month, e_dy);
    }
  }

  int workouts_done = 0;
  sqlite3_stmt *stmt_workouts;
  const char *sql_workouts = "SELECT COUNT(DISTINCT date(logged_at)) FROM workouts WHERE logged_at >= datetime('now', '-7 days', 'localtime');";
  if (sqlite3_prepare_v2(db, sql_workouts, -1, &stmt_workouts, NULL) == SQLITE_OK) {
    if (sqlite3_step(stmt_workouts) == SQLITE_ROW) {
      workouts_done = sqlite3_column_int(stmt_workouts, 0);
    }
    sqlite3_finalize(stmt_workouts);
  }

  double avg_protein = 0.0;
  sqlite3_stmt *stmt_prot;
  const char *sql_prot = "SELECT AVG(protein_g) FROM nutrition_log WHERE logged_at >= datetime('now', '-7 days', 'localtime');";
  if (sqlite3_prepare_v2(db, sql_prot, -1, &stmt_prot, NULL) == SQLITE_OK) {
    if (sqlite3_step(stmt_prot) == SQLITE_ROW) {
      avg_protein = sqlite3_column_double(stmt_prot, 0);
    }
    sqlite3_finalize(stmt_prot);
  }

  double avg_sleep = 0.0;
  sqlite3_stmt *stmt_sleep_val;
  const char *sql_sleep_val = "SELECT AVG(hours) FROM sleep_log WHERE logged_at >= datetime('now', '-7 days', 'localtime');";
  if (sqlite3_prepare_v2(db, sql_sleep_val, -1, &stmt_sleep_val, NULL) == SQLITE_OK) {
    if (sqlite3_step(stmt_sleep_val) == SQLITE_ROW) {
      avg_sleep = sqlite3_column_double(stmt_sleep_val, 0);
    }
    sqlite3_finalize(stmt_sleep_val);
  }

  double weight_new = 0.0;
  double weight_old = 0.0;
  sqlite3_stmt *stmt_wt_new;
  const char *sql_wt_new = "SELECT weight_kg FROM bodyweight ORDER BY logged_at DESC LIMIT 1;";
  if (sqlite3_prepare_v2(db, sql_wt_new, -1, &stmt_wt_new, NULL) == SQLITE_OK) {
    if (sqlite3_step(stmt_wt_new) == SQLITE_ROW) {
      weight_new = sqlite3_column_double(stmt_wt_new, 0);
    }
    sqlite3_finalize(stmt_wt_new);
  }

  sqlite3_stmt *stmt_wt_old;
  const char *sql_wt_old = "SELECT weight_kg FROM bodyweight WHERE logged_at < datetime('now', '-7 days', 'localtime') ORDER BY logged_at DESC LIMIT 1;";
  if (sqlite3_prepare_v2(db, sql_wt_old, -1, &stmt_wt_old, NULL) == SQLITE_OK) {
    if (sqlite3_step(stmt_wt_old) == SQLITE_ROW) {
      weight_old = sqlite3_column_double(stmt_wt_old, 0);
    }
    sqlite3_finalize(stmt_wt_old);
  }

  if (weight_old <= 0.0) {
    const char *sql_wt_oldest = "SELECT weight_kg FROM bodyweight ORDER BY logged_at ASC LIMIT 1;";
    sqlite3_stmt *stmt_wt_oldest;
    if (sqlite3_prepare_v2(db, sql_wt_oldest, -1, &stmt_wt_oldest, NULL) == SQLITE_OK) {
      if (sqlite3_step(stmt_wt_oldest) == SQLITE_ROW) {
        weight_old = sqlite3_column_double(stmt_wt_oldest, 0);
      }
      sqlite3_finalize(stmt_wt_oldest);
    }
  }

  print_logo();
  print_separator();

  printf("  week  %s\n", week_range);
  print_separator();

  // Print Workouts
  char wk_bar[64];
  render_progress_bar(wk_bar, sizeof(wk_bar), (double)workouts_done, 4.0, 8);
  double wk_pct = (double)workouts_done / 4.0 * 100.0;
  if (wk_pct > 100.0) wk_pct = 100.0;
  printf("  %-14s%-12s%s  %3.0f%%\n", "workouts", (workouts_done >= 4) ? "4 / 4" : (workouts_done == 3) ? "3 / 4" : (workouts_done == 2) ? "2 / 4" : (workouts_done == 1) ? "1 / 4" : "0 / 4", wk_bar, wk_pct);

  // Print Avg Protein
  char prot_bar[64];
  render_progress_bar(prot_bar, sizeof(prot_bar), avg_protein, 200.0, 8);
  double prot_pct = avg_protein / 200.0 * 100.0;
  if (prot_pct > 100.0) prot_pct = 100.0;
  char prot_ratio[32];
  snprintf(prot_ratio, sizeof(prot_ratio), "%.0f / 200g", avg_protein);
  double prot_diff = avg_protein - 200.0;
  char prot_diff_str[32] = "";
  if (prot_diff < 0.0) {
    snprintf(prot_diff_str, sizeof(prot_diff_str), NEGATIVE "%.0fg/day" RESET, prot_diff);
  } else if (prot_diff > 0.0) {
    snprintf(prot_diff_str, sizeof(prot_diff_str), POSITIVE "+%.0fg/day" RESET, prot_diff);
  } else {
    strcpy(prot_diff_str, "0g/day");
  }
  printf("  %-14s%-12s%s  %3.0f%%  %s\n", "avg protein", prot_ratio, prot_bar, prot_pct, prot_diff_str);

  // Print Avg Sleep
  char sleep_bar[64];
  render_progress_bar(sleep_bar, sizeof(sleep_bar), avg_sleep, 7.5, 8);
  double sl_pct = avg_sleep / 7.5 * 100.0;
  if (sl_pct > 100.0) sl_pct = 100.0;
  char sleep_ratio[32];
  snprintf(sleep_ratio, sizeof(sleep_ratio), "%.1f / 7.5h", avg_sleep);
  printf("  %-14s%-12s%s  %3.0f%%\n", "avg sleep", sleep_ratio, sleep_bar, sl_pct);

  // Print Bodyweight
  if (weight_old > 0.0 && weight_new > 0.0) {
    double diff = weight_new - weight_old;
    char bw_ratio[64];
    snprintf(bw_ratio, sizeof(bw_ratio), "%.1f → %.1fkg", weight_old, weight_new);
    if (diff < 0.0) {
      printf("  %-14s%-22s" POSITIVE "%.1fkg" RESET "\n", "bodyweight", bw_ratio, diff);
    } else if (diff > 0.0) {
      printf("  %-14s%-22s" NEGATIVE "+%.1fkg" RESET "\n", "bodyweight", bw_ratio, diff);
    } else {
      printf("  %-14s%-22s" DIM "0.0kg" RESET "\n", "bodyweight", bw_ratio);
    }
  } else {
    printf("  %-14s" DIM "—" RESET "\n", "bodyweight");
  }

  printf("\n  " ACCENT "prs this week" RESET "\n");
  int pr_count = 0;
  sqlite3_stmt *stmt_prs;
  const char *sql_prs = "SELECT exercise, weight_kg, reps FROM personal_records WHERE achieved_at >= datetime('now', '-7 days', 'localtime') ORDER BY achieved_at DESC;";
  if (sqlite3_prepare_v2(db, sql_prs, -1, &stmt_prs, NULL) == SQLITE_OK) {
    while (sqlite3_step(stmt_prs) == SQLITE_ROW) {
      const char *ex = (const char *)sqlite3_column_text(stmt_prs, 0);
      double wt = sqlite3_column_double(stmt_prs, 1);
      int reps = sqlite3_column_int(stmt_prs, 2);

      double prev_max = get_prev_max_weight(db, ex);
      double delta = wt - prev_max;

      printf("  %-18s%.1fkg × %d", ex, wt, reps);
      if (prev_max > 0.0 && delta > 0.0) {
        printf("  (" POSITIVE "+%.1fkg" RESET ")\n", delta);
      } else {
        printf("  (" POSITIVE "new" RESET ")\n");
      }
      pr_count++;
    }
    sqlite3_finalize(stmt_prs);
  }
  if (pr_count == 0) {
    printf("  " DIM "no new prs this week. keep pushing!" RESET "\n");
  }

  printf("\n");
  if (workouts_done >= 4 && avg_protein >= 200.0 && avg_sleep >= 7.5) {
    printf(POSITIVE "  ❯ solid week. all markers optimized. keep executing!" RESET "\n");
  } else if (workouts_done < 4) {
    printf(WARNING "  ❯ missed workouts target this week (%d/4) — focus on consistency and schedule blocking." RESET "\n", workouts_done);
  } else if (avg_protein < 190.0) {
    double deficit = 200.0 - avg_protein;
    printf(WARNING "  ❯ solid week. protein slightly under — add one meal or shake (-%.0fg/day)." RESET "\n", deficit);
  } else if (avg_sleep < 7.0) {
    printf(WARNING "  ❯ sleep compromised this week (avg %.1fh) — prioritize sleep hygiene to protect recovery." RESET "\n", avg_sleep);
  } else {
    printf(POSITIVE "  ❯ solid week. keep training hard and staying consistent!" RESET "\n");
  }

  print_separator();
}

static bool has_log_for_date(sqlite3 *db, const char *table, const char *date_str) {
  char sql[256];
  snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM %s WHERE date(logged_at) = date(?);", table);
  sqlite3_stmt *stmt;
  bool has_log = false;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, date_str, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      has_log = (sqlite3_column_int(stmt, 0) > 0);
    }
    sqlite3_finalize(stmt);
  }
  return has_log;
}

static void get_date_with_offset(sqlite3 *db, int offset, char *out_date, size_t max_len) {
  sqlite3_stmt *stmt;
  char sql[128];
  snprintf(sql, sizeof(sql), "SELECT date('now', 'localtime', '-%d days');", offset);
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      const char *d = (const char *)sqlite3_column_text(stmt, 0);
      if (d) {
        snprintf(out_date, max_len, "%s", d);
      }
    }
    sqlite3_finalize(stmt);
  }
}

static int calculate_streak(sqlite3 *db, const char *table) {
  char date_str[32];

  get_date_with_offset(db, 0, date_str, sizeof(date_str));
  bool logged_today = has_log_for_date(db, table, date_str);

  get_date_with_offset(db, 1, date_str, sizeof(date_str));
  bool logged_yesterday = has_log_for_date(db, table, date_str);

  if (!logged_today && !logged_yesterday) {
    return 0;
  }

  int start_offset = logged_today ? 0 : 1;
  int streak = 0;
  while (1) {
    get_date_with_offset(db, start_offset + streak, date_str, sizeof(date_str));
    if (has_log_for_date(db, table, date_str)) {
      streak++;
    } else {
      break;
    }
  }
  return streak;
}

static bool has_all_four_logs_for_date(sqlite3 *db, const char *date_str) {
  return has_log_for_date(db, "workouts", date_str) &&
         has_log_for_date(db, "sleep_log", date_str) &&
         has_log_for_date(db, "nutrition_log", date_str) &&
         has_log_for_date(db, "bodyweight", date_str);
}

static int calculate_overall_streak(sqlite3 *db) {
  char date_str[32];

  get_date_with_offset(db, 0, date_str, sizeof(date_str));
  bool logged_today = has_all_four_logs_for_date(db, date_str);

  get_date_with_offset(db, 1, date_str, sizeof(date_str));
  bool logged_yesterday = has_all_four_logs_for_date(db, date_str);

  if (!logged_today && !logged_yesterday) {
    return 0;
  }

  int start_offset = logged_today ? 0 : 1;
  int streak = 0;
  while (1) {
    get_date_with_offset(db, start_offset + streak, date_str, sizeof(date_str));
    if (has_all_four_logs_for_date(db, date_str)) {
      streak++;
    } else {
      break;
    }
  }
  return streak;
}

static void render_streak_bar(char *out_bar, size_t max_len, int streak) {
  char *p = out_bar;
  int limit = streak > 25 ? 25 : streak;
  for (int i = 0; i < limit && (p - out_bar) < (int)max_len - 4; i++) {
    strcpy(p, "█");
    p += 3;
  }
  *p = '\0';
}

void print_streak_dashboard(sqlite3 *db) {
  print_logo();
  print_separator();

  int workout_streak = calculate_streak(db, "workouts");
  int sleep_streak = calculate_streak(db, "sleep_log");
  int nutrition_streak = calculate_streak(db, "nutrition_log");
  int weight_streak = calculate_streak(db, "bodyweight");
  int overall_streak = calculate_overall_streak(db);

  char bar[128];
  char streak_str[32];

  render_streak_bar(bar, sizeof(bar), workout_streak);
  snprintf(streak_str, sizeof(streak_str), "%d day%s", workout_streak, workout_streak == 1 ? "" : "s");
  printf("  %-14s%-9s" ACCENT "%s" RESET "\n", "workout", streak_str, bar);

  render_streak_bar(bar, sizeof(bar), sleep_streak);
  snprintf(streak_str, sizeof(streak_str), "%d day%s", sleep_streak, sleep_streak == 1 ? "" : "s");
  printf("  %-14s%-9s" ACCENT "%s" RESET "\n", "sleep log", streak_str, bar);

  render_streak_bar(bar, sizeof(bar), nutrition_streak);
  snprintf(streak_str, sizeof(streak_str), "%d day%s", nutrition_streak, nutrition_streak == 1 ? "" : "s");
  printf("  %-14s%-9s" ACCENT "%s" RESET "\n", "nutrition", streak_str, bar);

  render_streak_bar(bar, sizeof(bar), weight_streak);
  snprintf(streak_str, sizeof(streak_str), "%d day%s", weight_streak, weight_streak == 1 ? "" : "s");
  printf("  %-14s%-9s" ACCENT "%s" RESET "\n", "bodyweight", streak_str, bar);

  printf("\n");

  snprintf(streak_str, sizeof(streak_str), "%d day%s", overall_streak, overall_streak == 1 ? "" : "s");
  printf("  %-14s%-9s" DIM "—" RESET " all four logged\n", "overall", streak_str);

  print_separator();
}

void print_prs_dashboard(sqlite3 *db) {
  print_logo();
  print_separator();

  printf(DIM "  %-18s%-10s%-7s%-11s%s\n" RESET, "exercise", "weight", "reps", "est. 1rm", "date");
  print_thin_sep();

  sqlite3_stmt *stmt;
  const char *sql = "SELECT exercise, weight_kg, reps, estimated_1rm, achieved_at FROM personal_records ORDER BY achieved_at DESC, estimated_1rm DESC;";
  int count = 0;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      const char *ex = (const char *)sqlite3_column_text(stmt, 0);
      double weight = sqlite3_column_double(stmt, 1);
      int reps = sqlite3_column_int(stmt, 2);
      double orm = sqlite3_column_double(stmt, 3);
      const char *date_str = (const char *)sqlite3_column_text(stmt, 4);

      char display_date[32] = "N/A";
      if (date_str) {
        int yr = 0, mo = 0, dy = 0;
        if (sscanf(date_str, "%d-%d-%d", &yr, &mo, &dy) == 3) {
          const char *months_names[] = {
              "jan", "feb", "mar", "apr", "may", "jun",
              "jul", "aug", "sep", "oct", "nov", "dec"
          };
          const char *month_name = (mo >= 1 && mo <= 12) ? months_names[mo - 1] : "unk";
          snprintf(display_date, sizeof(display_date), "%s %02d", month_name, dy);
        }
      }

      char wt_str[32];
      if (weight == (int)weight) {
        snprintf(wt_str, sizeof(wt_str), "%.0fkg", weight);
      } else {
        snprintf(wt_str, sizeof(wt_str), "%.1fkg", weight);
      }

      char orm_str[32];
      if (orm == (int)orm) {
        snprintf(orm_str, sizeof(orm_str), "%.0fkg", orm);
      } else {
        snprintf(orm_str, sizeof(orm_str), "%.1fkg", orm);
      }

      char reps_str[16];
      snprintf(reps_str, sizeof(reps_str), "%d", reps);

      printf("  %-18s%-10s%-7s%-11s%s\n", ex, wt_str, reps_str, orm_str, display_date);
      count++;
    }
    sqlite3_finalize(stmt);
  }

  if (count == 0) {
    printf(DIM "  no personal records logged yet. go set some!" RESET "\n");
  }

  print_separator();
}
