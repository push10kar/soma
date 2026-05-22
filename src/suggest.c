#include "suggest.h"
#include "utils.h"
#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

static void parse_last_set(const char *w_str, const char *r_str, double *out_w, int *out_r) {
  if (!w_str || !r_str) return;

  // Extract last weight in CSV string
  size_t w_len = strlen(w_str) + 1;
  char *dup_w = (char *)malloc(w_len);
  if (dup_w) {
    memcpy(dup_w, w_str, w_len);
    char *tok_w = strtok(dup_w, ",");
    while (tok_w) {
      *out_w = atof(tok_w);
      tok_w = strtok(NULL, ",");
    }
    free(dup_w);
  }

  // Extract last reps in CSV string
  size_t r_len = strlen(r_str) + 1;
  char *dup_r = (char *)malloc(r_len);
  if (dup_r) {
    memcpy(dup_r, r_str, r_len);
    char *tok_r = strtok(dup_r, ",");
    while (tok_r) {
      *out_r = atoi(tok_r);
      tok_r = strtok(NULL, ",");
    }
    free(dup_r);
  }
}

static bool get_30d_1rm_stats(sqlite3 *db, const char *exercise,
                              double *out_old_1rm, double *out_new_1rm,
                              double *out_diff, double *out_pct,
                              char *out_old_date, char *out_new_date,
                              double *out_latest_w, int *out_latest_r) {
  sqlite3_stmt *stmt;
  const char *sql_latest =
      "SELECT weight_kg, reps, logged_at FROM workouts "
      "WHERE LOWER(exercise) = LOWER(?) "
      "ORDER BY logged_at DESC, id DESC LIMIT 1;";

  const char *sql_oldest_30d =
      "SELECT weight_kg, reps, logged_at FROM workouts "
      "WHERE LOWER(exercise) = LOWER(?) AND logged_at >= datetime('now', '-30 days', 'localtime') "
      "ORDER BY logged_at ASC, id ASC LIMIT 1;";

  double latest_w = 0.0, oldest_w = 0.0;
  int latest_r = 0, oldest_r = 0;
  char latest_date[64] = "N/A", oldest_date[64] = "N/A";
  bool found_latest = false, found_oldest = false;

  // 1. Fetch latest workout session details
  if (sqlite3_prepare_v2(db, sql_latest, -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, exercise, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      const char *w_str = (const char *)sqlite3_column_text(stmt, 0);
      const char *r_str = (const char *)sqlite3_column_text(stmt, 1);
      const char *date = (const char *)sqlite3_column_text(stmt, 2);
      parse_last_set(w_str, r_str, &latest_w, &latest_r);
      if (date) snprintf(latest_date, sizeof(latest_date), "%.10s", date);
      found_latest = true;
    }
    sqlite3_finalize(stmt);
  }

  if (!found_latest) return false;

  // 2. Fetch oldest workout session within the 30-day window
  if (sqlite3_prepare_v2(db, sql_oldest_30d, -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, exercise, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      const char *w_str = (const char *)sqlite3_column_text(stmt, 0);
      const char *r_str = (const char *)sqlite3_column_text(stmt, 1);
      const char *date = (const char *)sqlite3_column_text(stmt, 2);
      parse_last_set(w_str, r_str, &oldest_w, &oldest_r);
      if (date) snprintf(oldest_date, sizeof(oldest_date), "%.10s", date);
      found_oldest = true;
    }
    sqlite3_finalize(stmt);
  }

  // Fallback: If only 1 workout is logged in the 30d window, compare against the absolute oldest session
  if (!found_oldest || strcmp(latest_date, oldest_date) == 0) {
    const char *sql_alltime_oldest =
        "SELECT weight_kg, reps, logged_at FROM workouts "
        "WHERE LOWER(exercise) = LOWER(?) "
        "ORDER BY logged_at ASC, id ASC LIMIT 1;";
    if (sqlite3_prepare_v2(db, sql_alltime_oldest, -1, &stmt, NULL) == SQLITE_OK) {
      sqlite3_bind_text(stmt, 1, exercise, -1, SQLITE_STATIC);
      if (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *w_str = (const char *)sqlite3_column_text(stmt, 0);
        const char *r_str = (const char *)sqlite3_column_text(stmt, 1);
        const char *date = (const char *)sqlite3_column_text(stmt, 2);
        parse_last_set(w_str, r_str, &oldest_w, &oldest_r);
        if (date) snprintf(oldest_date, sizeof(oldest_date), "%.10s", date);
        found_oldest = true;
      }
      sqlite3_finalize(stmt);
    }
  }

  // If still no older baseline session found, default to latest (0% progress)
  if (!found_oldest) {
    oldest_w = latest_w;
    oldest_r = latest_r;
    strcpy(oldest_date, latest_date);
  }

  double latest_1rm = latest_w * (1.0 + latest_r / 30.0);
  double oldest_1rm = oldest_w * (1.0 + oldest_r / 30.0);

  *out_old_1rm = oldest_1rm;
  *out_new_1rm = latest_1rm;
  *out_diff = latest_1rm - oldest_1rm;
  if (oldest_1rm > 0.0) {
    *out_pct = (*out_diff / oldest_1rm) * 100.0;
  } else {
    *out_pct = 0.0;
  }

  if (out_old_date) strcpy(out_old_date, oldest_date);
  if (out_new_date) strcpy(out_new_date, latest_date);
  if (out_latest_w) *out_latest_w = latest_w;
  if (out_latest_r) *out_latest_r = latest_r;

  return true;
}

static int get_visible_len(const char *s) {
  int len = 0;
  bool in_esc = false;
  for (int i = 0; s[i] != '\0'; i++) {
    if (s[i] == '\033') {
      in_esc = true;
    } else if (in_esc) {
      if (s[i] == 'm') {
        in_esc = false;
      }
    } else {
      len++;
    }
  }
  return len;
}

static bool exercise_matches(const char *ex_name, const char *filter) {
  if (!filter) return true;

  char ex_low[128];
  char filt_low[128];

  int i;
  for (i = 0; ex_name[i] != '\0' && i < 127; i++) {
    char c = ex_name[i];
    if (c >= 'A' && c <= 'Z') c = c + 32;
    ex_low[i] = c;
  }
  ex_low[i] = '\0';

  for (i = 0; filter[i] != '\0' && i < 127; i++) {
    char c = filter[i];
    if (c >= 'A' && c <= 'Z') c = c + 32;
    filt_low[i] = c;
  }
  filt_low[i] = '\0';

  // Map alias OHP -> overhead press
  if (strcmp(filt_low, "ohp") == 0 && strcmp(ex_low, "overhead press") == 0) {
    return true;
  }

  if (strstr(ex_low, filt_low) != NULL || strstr(filt_low, ex_low) != NULL) {
    return true;
  }

  return false;
}

static double calculate_slope(double *y, int n) {
  if (n < 2) return 0.0;
  double sum_x = 0.0;
  double sum_y = 0.0;
  double sum_xx = 0.0;
  double sum_xy = 0.0;
  for (int i = 0; i < n; i++) {
    double x = (double)i;
    sum_x += x;
    sum_y += y[i];
    sum_xx += x * x;
    sum_xy += x * y[i];
  }
  double denom = n * sum_xx - sum_x * sum_x;
  if (denom == 0.0) return 0.0;
  return (n * sum_xy - sum_x * sum_y) / denom;
}

typedef struct {
  double weight;
  int reps;
  double vol;
  double est_1rm;
  char date[32];
  long long timestamp;
} SessionData;

static int get_last_4_sessions(sqlite3 *db, const char *exercise, SessionData *sessions, int max_sessions) {
  sqlite3_stmt *stmt;
  const char *sql =
      "SELECT weight_kg, reps, volume_kg, logged_at, strftime('%s', logged_at) FROM workouts "
      "WHERE LOWER(exercise) = LOWER(?) "
      "ORDER BY logged_at DESC, id DESC LIMIT ?;";

  int count = 0;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, exercise, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, max_sessions);

    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_sessions) {
      const char *w_str = (const char *)sqlite3_column_text(stmt, 0);
      const char *r_str = (const char *)sqlite3_column_text(stmt, 1);
      double vol = sqlite3_column_double(stmt, 2);
      const char *date = (const char *)sqlite3_column_text(stmt, 3);
      long long ts = sqlite3_column_int64(stmt, 4);

      double w_val = 0.0;
      int r_val = 0;
      parse_last_set(w_str, r_str, &w_val, &r_val);

      sessions[count].weight = w_val;
      sessions[count].reps = r_val;
      sessions[count].vol = vol;
      sessions[count].est_1rm = w_val * (1.0 + r_val / 30.0);
      if (date) {
        snprintf(sessions[count].date, sizeof(sessions[count].date), "%.10s", date);
      } else {
        strcpy(sessions[count].date, "N/A");
      }
      sessions[count].timestamp = ts;
      count++;
    }
    sqlite3_finalize(stmt);
  }

  // Reverse so sessions are in chronological order (oldest to newest)
  for (int i = 0; i < count / 2; i++) {
    SessionData temp = sessions[i];
    sessions[i] = sessions[count - 1 - i];
    sessions[count - 1 - i] = temp;
  }

  return count;
}

void print_suggestions(sqlite3 *db, const char *filter_exercise) {
  print_logo();
  print_separator();

  printf(ACCENT "  ▸ workload & target suggestions" RESET "\n\n");
  printf(DIM "  %-18s%-24s%-20s%s\n\n" RESET,
         "exercise", "30d 1rm progress", "target set", "coaching recommendation");

  const char *exercises[] = {"bench press", "squat", "overhead press"};
  int num_exercises = 3;

  // Interrogate recovery telemetry (7-day sleep averages)
  double sleep_avg = db_get_sleep_7d_avg(db);
  bool sleep_low = (sleep_avg > 0.0 && sleep_avg < 7.0);

  for (int i = 0; i < num_exercises; i++) {
    const char *ex = exercises[i];
    if (filter_exercise && !exercise_matches(ex, filter_exercise)) {
      continue;
    }

    double old_1rm = 0.0, new_1rm = 0.0, diff = 0.0, pct = 0.0;
    char old_date[64] = "", new_date[64] = "";
    double latest_w = 0.0;
    int latest_r = 0;

    if (get_30d_1rm_stats(db, ex, &old_1rm, &new_1rm, &diff, &pct, old_date, new_date, &latest_w, &latest_r)) {
      char prog_str[64];
      if (diff > 0.0) {
        snprintf(prog_str, sizeof(prog_str), POSITIVE "+%.1fkg (+%.1f%%)" RESET, diff, pct);
      } else if (diff < 0.0) {
        snprintf(prog_str, sizeof(prog_str), NEGATIVE "%.1fkg (%.1f%%)" RESET, diff, pct);
      } else {
        snprintf(prog_str, sizeof(prog_str), DIM "— (0.0%%)" RESET);
      }

      // Linear and autoregulated progression targets based on last 4 sessions
      SessionData last_4[4];
      int count_4 = get_last_4_sessions(db, ex, last_4, 4);

      double target_w = latest_w;
      int target_r = latest_r;
      char recommendation[256] = "";

      if (count_4 >= 2) {
        double est_1rms[4];
        for (int j = 0; j < count_4; j++) {
          est_1rms[j] = last_4[j].est_1rm;
        }
        double slope = calculate_slope(est_1rms, count_4);
        
        // Calculate time difference between oldest and newest in days
        double days_diff = (double)(last_4[count_4 - 1].timestamp - last_4[0].timestamp) / 86400.0;

        if (sleep_low) {
          if (strcasecmp(ex, "squat") == 0) {
            target_w = latest_w * 0.90; // Autoregulated Squat Deload
            snprintf(recommendation, sizeof(recommendation), WARNING "CNS fatigue — execute velocity deep squat deload" RESET);
          } else {
            target_w = latest_w * 0.95; // Autoregulated Deload
            snprintf(recommendation, sizeof(recommendation), WARNING "sleep compromised — recommend deload -5%%" RESET);
          }
        } else if (slope < -0.15) {
          // Regressing
          target_w = latest_w;
          snprintf(recommendation, sizeof(recommendation), NEGATIVE "⚠️ REGRESSING - strength is dropping. check sleep & nutrition" RESET);
        } else if (slope >= -0.15 && slope <= 0.15) {
          // Stalled
          if (days_diff >= 14.0) {
            // Stalled 2+ weeks
            target_w = latest_w * 0.90; // Recommend 10% deload
            if (strcasecmp(ex, "bench press") == 0) {
              snprintf(recommendation, sizeof(recommendation), WARNING "stalled 2+ wks — suggest close-grip variation or -10%% deload" RESET);
            } else if (strcasecmp(ex, "squat") == 0) {
              snprintf(recommendation, sizeof(recommendation), WARNING "stalled 2+ wks — suggest pause squats or -10%% deload" RESET);
            } else {
              snprintf(recommendation, sizeof(recommendation), WARNING "stalled 2+ wks — suggest push press variation or -10%% deload" RESET);
            }
          } else {
            // Stalled but less than 2 weeks
            target_w = latest_w;
            if (strcasecmp(ex, "bench press") == 0) {
              snprintf(recommendation, sizeof(recommendation), WARNING "stalled — attempt reps PR at current weight" RESET);
            } else if (strcasecmp(ex, "squat") == 0) {
              snprintf(recommendation, sizeof(recommendation), WARNING "stalled — focus on absolute hip crease velocity" RESET);
            } else {
              snprintf(recommendation, sizeof(recommendation), WARNING "stalled — add 1 extra rep to set" RESET);
            }
          }
        } else {
          // Progressing
          double increment = (strcasecmp(ex, "overhead press") == 0) ? 1.0 : 2.5;
          target_w = latest_w + increment;
          if (strcasecmp(ex, "bench press") == 0) {
            snprintf(recommendation, sizeof(recommendation), POSITIVE "progressing well — attempt +2.5kg target" RESET);
          } else if (strcasecmp(ex, "squat") == 0) {
            snprintf(recommendation, sizeof(recommendation), POSITIVE "knees adapted — step weight up by +2.5kg" RESET);
          } else {
            snprintf(recommendation, sizeof(recommendation), POSITIVE "shoulder stable — attempt microload +1.0kg" RESET);
          }
        }
      } else {
        // Fallback for insufficient history
        if (sleep_low) {
          target_w = latest_w * 0.95;
          snprintf(recommendation, sizeof(recommendation), WARNING "sleep compromised — recommend deload -5%%" RESET);
        } else {
          target_w = latest_w;
          snprintf(recommendation, sizeof(recommendation), DIM "insufficient history — attempt current workload" RESET);
        }
      }

      int vis_len = get_visible_len(prog_str);
      int target_pad = 24;
      int pad_len = target_pad - vis_len;
      if (pad_len < 1) pad_len = 1;

      char pad_spaces[64];
      memset(pad_spaces, ' ', pad_len);
      pad_spaces[pad_len] = '\0';

      char target_str[64];
      snprintf(target_str, sizeof(target_str), "%.1fkg × %d", target_w, target_r);

      printf("  %-18s%s%s%-20s%s\n\n",
             ex, prog_str, pad_spaces, target_str, recommendation);

    } else {
      printf("  %-18s" DIM "%-24s%-20s%s" RESET "\n\n",
             ex, "no history", "—", "log workouts to generate recommendations");
    }
  }

  print_separator();
  if (sleep_low) {
    printf(WARNING "  ❯ deload flags raised due to low recovery (7d sleep avg: %.1fh). priority is sleep!" RESET "\n", sleep_avg);
  } else {
    printf(POSITIVE "  ❯ recovery fully optimized (7d sleep avg: %.1fh). execution mode: progress targets!" RESET "\n", sleep_avg);
  }
  print_separator();
}

