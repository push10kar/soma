#include "../include/db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* Static string copy helper for standard ISO C compliance */
static char *str_dup(const char *str) {
  if (!str)
    return NULL;
  size_t len = strlen(str) + 1;
  char *dup = (char *)malloc(len);
  if (dup)
    memcpy(dup, str, len);
  return dup;
}

sqlite3 *db_open(void) {

  char dir[512];
  char path[512];

  const char *home = getenv("HOME");
  if (!home) {
    fprintf(stderr, "error: HOME not set\n");
    exit(1);
  }

  snprintf(dir, sizeof(dir), "%s/.soma", home);
  snprintf(path, sizeof(path), "%s/.soma/soma.db", home);

  /* create ~/.soma/ if it doesn't exist */
  mkdir(dir, 0755);

  sqlite3 *db;
  if (sqlite3_open(path, &db) != SQLITE_OK) {
    fprintf(stderr, "error: cannot open database: %s\n", sqlite3_errmsg(db));
    exit(1);
  }

  db_init(db);
  return db;
}

void db_close(sqlite3 *db) { sqlite3_close(db); }

void db_init(sqlite3 *db) {
  const char *sql =
      "CREATE TABLE IF NOT EXISTS workouts ("
      "  id          INTEGER PRIMARY KEY,"
      "  exercise    TEXT    NOT NULL,"
      "  weight_kg   TEXT    NOT NULL,"
      "  reps        TEXT    NOT NULL,"
      "  volume_kg   REAL    NOT NULL,"
      "  logged_at   TEXT    DEFAULT (datetime('now','localtime'))"
      ");"

      "CREATE TABLE IF NOT EXISTS personal_records ("
      "  exercise      TEXT PRIMARY KEY,"
      "  weight_kg     REAL    NOT NULL,"
      "  reps          INTEGER NOT NULL,"
      "  estimated_1rm REAL,"
      "  is_new_pr     INTEGER DEFAULT 0,"
      "  achieved_at   TEXT    NOT NULL"
      ");"

      "CREATE TABLE IF NOT EXISTS bodyweight ("
      "  id          INTEGER PRIMARY KEY,"
      "  weight_kg   REAL NOT NULL,"
      "  waist_cm    REAL,"
      "  logged_at   TEXT DEFAULT (datetime('now','localtime'))"
      ");"

      "CREATE TABLE IF NOT EXISTS sleep_log ("
      "  id          INTEGER PRIMARY KEY,"
      "  hours       REAL    NOT NULL,"
      "  quality     INTEGER,"
      "  logged_at   TEXT    DEFAULT (datetime('now','localtime'))"
      ");"

      "CREATE TABLE IF NOT EXISTS nutrition_log ("
      "  id          INTEGER PRIMARY KEY,"
      "  meal        TEXT    NOT NULL,"
      "  protein_g   REAL    NOT NULL,"
      "  carbs_g     REAL    NOT NULL,"
      "  fat_g       REAL    NOT NULL,"
      "  calories    INTEGER NOT NULL,"
      "  logged_at   TEXT    DEFAULT (datetime('now','localtime'))"
      ");"

      "CREATE TABLE IF NOT EXISTS meal_templates ("
      "  name        TEXT PRIMARY KEY,"
      "  protein_g   REAL    NOT NULL,"
      "  carbs_g     REAL    NOT NULL,"
      "  fat_g       REAL    NOT NULL,"
      "  calories    INTEGER NOT NULL"
      ");";

  char *err = NULL;
  if (sqlite3_exec(db, sql, NULL, NULL, &err) != SQLITE_OK) {
    fprintf(stderr, "error: db init failed: %s\n", err);
    sqlite3_free(err);
    exit(1);
  }

  /* Safe migration: add waist_cm to bodyweight table silently if it doesn't
   * exist */
  sqlite3_exec(db, "ALTER TABLE bodyweight ADD COLUMN waist_cm REAL;", NULL,
               NULL, NULL);
}

/* Seeding Engine - populates high-fidelity training data on empty boot */
void db_seed(sqlite3 *db) {
  sqlite3_stmt *stmt;
  int count = 0;

  if (sqlite3_prepare_v2(db, "SELECT COUNT(*) FROM workouts;", -1, &stmt,
                         NULL) == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
  }

  /* Seed only if database has no workouts */
  if (count == 0) {
    fprintf(stderr, "DEBUG: Seeding database with initial high-fidelity "
                    "training logs...\n");
    const char *seed_sql =
        /* Workouts - Bench Press */
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('bench press', '72.5', '5', 362.5, datetime('now', "
        "'-20 days', 'localtime'));"
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('bench press', '75.0', '5', 375.0, datetime('now', "
        "'-16 days', 'localtime'));"
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('bench press', '75.0', '5', 375.0, datetime('now', "
        "'-12 days', 'localtime'));"
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('bench press', '77.5', '5', 387.5, datetime('now', "
        "'-8 days', 'localtime'));"
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('bench press', '77.5', '5', 387.5, datetime('now', "
        "'-4 days', 'localtime'));"
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('bench press', '80.0', '5', 400.0, datetime('now', "
        "'-1 day', 'localtime'));"

        /* Workouts - Squat */
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('squat', '100.0', '5', 500.0, datetime('now', '-20 "
        "days', 'localtime'));"
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('squat', '105.0', '5', 525.0, datetime('now', '-16 "
        "days', 'localtime'));"
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('squat', '107.5', '5', 537.5, datetime('now', '-12 "
        "days', 'localtime'));"
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('squat', '110.0', '5', 550.0, datetime('now', '-8 "
        "days', 'localtime'));"
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('squat', '115.0', '5', 575.0, datetime('now', '-4 "
        "days', 'localtime'));"
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('squat', '120.0', '5', 600.0, datetime('now', '-1 "
        "day', 'localtime'));"

        /* Workouts - Overhead Press */
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('overhead press', '45.0', '5', 225.0, "
        "datetime('now', '-20 days', 'localtime'));"
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('overhead press', '47.5', '5', 237.5, "
        "datetime('now', '-16 days', 'localtime'));"
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('overhead press', '47.5', '5', 237.5, "
        "datetime('now', '-12 days', 'localtime'));"
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('overhead press', '50.0', '5', 250.0, "
        "datetime('now', '-8 days', 'localtime'));"
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('overhead press', '50.0', '5', 250.0, "
        "datetime('now', '-4 days', 'localtime'));"
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg, "
        "logged_at) VALUES ('overhead press', '52.0', '5', 260.0, "
        "datetime('now', '-1 day', 'localtime'));"

        /* Personal Records */
        "INSERT OR REPLACE INTO personal_records (exercise, weight_kg, reps, "
        "estimated_1rm, is_new_pr, achieved_at) VALUES ('bench press', 80.0, "
        "5, 93.3, 1, datetime('now', '-1 day', 'localtime'));"
        "INSERT OR REPLACE INTO personal_records (exercise, weight_kg, reps, "
        "estimated_1rm, is_new_pr, achieved_at) VALUES ('squat', 120.0, 5, "
        "139.9, 1, datetime('now', '-1 day', 'localtime'));"
        "INSERT OR REPLACE INTO personal_records (exercise, weight_kg, reps, "
        "estimated_1rm, is_new_pr, achieved_at) VALUES ('overhead press', "
        "52.0, 5, 60.4, 1, datetime('now', '-1 day', 'localtime'));"

        /* Bodyweight and Waist */
        "INSERT INTO bodyweight (weight_kg, waist_cm, logged_at) VALUES (81.0, "
        "85.0, datetime('now', '-25 days', 'localtime'));"
        "INSERT INTO bodyweight (weight_kg, waist_cm, logged_at) VALUES (80.5, "
        "84.5, datetime('now', '-20 days', 'localtime'));"
        "INSERT INTO bodyweight (weight_kg, waist_cm, logged_at) VALUES (80.2, "
        "84.0, datetime('now', '-15 days', 'localtime'));"
        "INSERT INTO bodyweight (weight_kg, waist_cm, logged_at) VALUES (79.8, "
        "83.8, datetime('now', '-10 days', 'localtime'));"
        "INSERT INTO bodyweight (weight_kg, waist_cm, logged_at) VALUES (79.5, "
        "83.5, datetime('now', '-5 days', 'localtime'));"
        "INSERT INTO bodyweight (weight_kg, waist_cm, logged_at) VALUES (79.2, "
        "83.0, datetime('now', '-1 day', 'localtime'));"

        /* Sleep */
        "INSERT INTO sleep_log (hours, quality, logged_at) VALUES (6.5, 3, "
        "datetime('now', '-6 days', 'localtime'));"
        "INSERT INTO sleep_log (hours, quality, logged_at) VALUES (7.0, 4, "
        "datetime('now', '-5 days', 'localtime'));"
        "INSERT INTO sleep_log (hours, quality, logged_at) VALUES (7.5, 4, "
        "datetime('now', '-4 days', 'localtime'));"
        "INSERT INTO sleep_log (hours, quality, logged_at) VALUES (6.0, 2, "
        "datetime('now', '-3 days', 'localtime'));"
        "INSERT INTO sleep_log (hours, quality, logged_at) VALUES (7.5, 4, "
        "datetime('now', '-2 days', 'localtime'));"
        "INSERT INTO sleep_log (hours, quality, logged_at) VALUES (8.0, 5, "
        "datetime('now', '-1 day', 'localtime'));"

        /* Today's Meals - Nutrition Log */
        "INSERT INTO nutrition_log (meal, protein_g, carbs_g, fat_g, calories, "
        "logged_at) VALUES ('oatmeal with whey', 40.0, 60.0, 8.0, 450, "
        "datetime('now', 'localtime'));"
        "INSERT INTO nutrition_log (meal, protein_g, carbs_g, fat_g, calories, "
        "logged_at) VALUES ('chicken breast and rice', 65.0, 80.0, 10.0, 700, "
        "datetime('now', 'localtime'));"
        "INSERT INTO nutrition_log (meal, protein_g, carbs_g, fat_g, calories, "
        "logged_at) VALUES ('protein shake & almonds', 35.0, 15.0, 18.0, 350, "
        "datetime('now', 'localtime'));"
        "INSERT INTO nutrition_log (meal, protein_g, carbs_g, fat_g, calories, "
        "logged_at) VALUES ('steak and sweet potato', 38.0, 45.0, 19.0, 600, "
        "datetime('now', 'localtime'));";

    sqlite3_exec(db, seed_sql, NULL, NULL, NULL);
  }
}

/* Data Binding Queries - retrieve actual, dynamic fitness telemetry */

int db_get_exercise_history(sqlite3 *db, const char *exercise,
                            double *out_weights, int *out_reps,
                            double *out_volumes, int max_count) {
  sqlite3_stmt *stmt;
  /* Fetch last max_count records in ascending order of logging so they graph
   * left-to-right */
  const char *sql =
      "SELECT weight_kg, reps, volume_kg FROM ("
      "  SELECT weight_kg, reps, volume_kg, logged_at, id FROM workouts "
      "  WHERE LOWER(exercise) = LOWER(?)"
      "  ORDER BY logged_at DESC, id DESC LIMIT ?"
      ") ORDER BY logged_at ASC, id ASC;";

  int count = 0;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, exercise, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, max_count);

    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_count) {
      const char *w_str = (const char *)sqlite3_column_text(stmt, 0);
      const char *r_str = (const char *)sqlite3_column_text(stmt, 1);
      double vol = sqlite3_column_double(stmt, 2);

      /* For charting, extract the last (or highest) weight & rep from the csv
       * string */
      double w_val = 0;
      if (w_str) {
        char *dup = str_dup(w_str);
        char *tok = strtok(dup, ",");
        while (tok) {
          w_val = atof(tok); /* keeps the last weight in set */
          tok = strtok(NULL, ",");
        }
        free(dup);
      }

      int r_val = 0;
      if (r_str) {
        char *dup = str_dup(r_str);
        char *tok = strtok(dup, ",");
        while (tok) {
          r_val = atoi(tok); /* keeps the last rep in set */
          tok = strtok(NULL, ",");
        }
        free(dup);
      }

      if (out_weights)
        out_weights[count] = w_val;
      if (out_reps)
        out_reps[count] = r_val;
      if (out_volumes)
        out_volumes[count] = vol;
      count++;
    }
    sqlite3_finalize(stmt);
  }
  return count;
}

bool db_get_latest_workout_set(sqlite3 *db, const char *exercise,
                               double *out_weight, int *out_reps) {
  sqlite3_stmt *stmt;
  const char *sql = "SELECT weight_kg, reps FROM workouts "
                    "WHERE LOWER(exercise) = LOWER(?)"
                    "ORDER BY logged_at DESC, id DESC LIMIT 1;";

  bool found = false;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, exercise, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
      const char *w_str = (const char *)sqlite3_column_text(stmt, 0);
      const char *r_str = (const char *)sqlite3_column_text(stmt, 1);

      double w_val = 0;
      if (w_str) {
        char *dup = str_dup(w_str);
        char *tok = strtok(dup, ",");
        while (tok) {
          w_val = atof(tok);
          tok = strtok(NULL, ",");
        }
        free(dup);
      }

      int r_val = 0;
      if (r_str) {
        char *dup = str_dup(r_str);
        char *tok = strtok(dup, ",");
        while (tok) {
          r_val = atoi(tok);
          tok = strtok(NULL, ",");
        }
        free(dup);
      }

      if (out_weight)
        *out_weight = w_val;
      if (out_reps)
        *out_reps = r_val;
      found = true;
    }
    sqlite3_finalize(stmt);
  }
  return found;
}

double db_get_estimated_1rm(sqlite3 *db, const char *exercise) {
  sqlite3_stmt *stmt;
  double e_1rm = 0.0;
  const char *sql = "SELECT estimated_1rm FROM personal_records WHERE "
                    "LOWER(exercise) = LOWER(?);";

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, exercise, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      e_1rm = sqlite3_column_double(stmt, 0);
    }
    sqlite3_finalize(stmt);
  }

  /* Fallback Epley formula if personal records is missing but workouts exist */
  if (e_1rm <= 0.0) {
    double weight = 0.0;
    int reps = 0;
    if (db_get_latest_workout_set(db, exercise, &weight, &reps) && reps > 0) {
      e_1rm = weight * (1.0 + reps / 30.0);
    }
  }
  return e_1rm;
}

int db_get_bodyweight_history(sqlite3 *db, double *out_weights,
                              double *out_waist, int max_count) {
  sqlite3_stmt *stmt;
  const char *sql =
      "SELECT weight_kg, waist_cm FROM ("
      "  SELECT weight_kg, waist_cm, logged_at, id FROM bodyweight "
      "  ORDER BY logged_at DESC, id DESC LIMIT ?"
      ") ORDER BY logged_at ASC, id ASC;";

  int count = 0;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, max_count);
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_count) {
      if (out_weights)
        out_weights[count] = sqlite3_column_double(stmt, 0);
      if (out_waist)
        out_waist[count] = sqlite3_column_double(stmt, 1);
      count++;
    }
    sqlite3_finalize(stmt);
  }
  return count;
}

double db_get_bodyweight_weekly_change(sqlite3 *db) {
  sqlite3_stmt *stmt;
  /* Compare the latest weight against one from ~7 days ago, or the earliest
   * available older entry */
  const char *sql = "SELECT weight_kg, logged_at FROM bodyweight "
                    "ORDER BY logged_at DESC, id DESC;";

  double latest_w = 0.0;
  double older_w = 0.0;
  double days_diff = 0.0;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    int idx = 0;
    char latest_date[64] = "";

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      double w = sqlite3_column_double(stmt, 0);
      const char *date = (const char *)sqlite3_column_text(stmt, 1);

      if (idx == 0) {
        latest_w = w;
        if (date)
          snprintf(latest_date, sizeof(latest_date), "%s", date);
      } else {
        older_w = w;
        /* Calculate days difference between older and latest */
        if (date && latest_date[0] != '\0') {
          sqlite3_stmt *stmt_diff;
          const char *diff_sql = "SELECT julianday(?) - julianday(?);";
          if (sqlite3_prepare_v2(db, diff_sql, -1, &stmt_diff, NULL) ==
              SQLITE_OK) {
            sqlite3_bind_text(stmt_diff, 1, latest_date, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt_diff, 2, date, -1, SQLITE_STATIC);
            if (sqlite3_step(stmt_diff) == SQLITE_ROW) {
              days_diff = sqlite3_column_double(stmt_diff, 0);
            }
            sqlite3_finalize(stmt_diff);
          }
        }
        /* We break early if we found an entry roughly 4-10 days older */
        if (days_diff >= 3.0) {
          break;
        }
      }
      idx++;
    }
    sqlite3_finalize(stmt);
  }

  if (days_diff > 0.5 && older_w > 0.0) {
    return ((latest_w - older_w) / days_diff) * 7.0; /* weekly change rate */
  }
  return 0.0;
}

double db_get_waist_weekly_change(sqlite3 *db) {
  sqlite3_stmt *stmt;
  const char *sql = "SELECT waist_cm, logged_at FROM bodyweight "
                    "WHERE waist_cm IS NOT NULL "
                    "ORDER BY logged_at DESC, id DESC;";

  double latest_w = 0.0;
  double older_w = 0.0;
  double days_diff = 0.0;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    int idx = 0;
    char latest_date[64] = "";

    while (sqlite3_step(stmt) == SQLITE_ROW) {
      double w = sqlite3_column_double(stmt, 0);
      const char *date = (const char *)sqlite3_column_text(stmt, 1);

      if (idx == 0) {
        latest_w = w;
        if (date)
          snprintf(latest_date, sizeof(latest_date), "%s", date);
      } else {
        older_w = w;
        if (date && latest_date[0] != '\0') {
          sqlite3_stmt *stmt_diff;
          const char *diff_sql = "SELECT julianday(?) - julianday(?);";
          if (sqlite3_prepare_v2(db, diff_sql, -1, &stmt_diff, NULL) ==
              SQLITE_OK) {
            sqlite3_bind_text(stmt_diff, 1, latest_date, -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt_diff, 2, date, -1, SQLITE_STATIC);
            if (sqlite3_step(stmt_diff) == SQLITE_ROW) {
              days_diff = sqlite3_column_double(stmt_diff, 0);
            }
            sqlite3_finalize(stmt_diff);
          }
        }
        if (days_diff >= 3.0) {
          break;
        }
      }
      idx++;
    }
    sqlite3_finalize(stmt);
  }

  if (days_diff > 0.5 && older_w > 0.0) {
    return ((latest_w - older_w) / days_diff) * 7.0;
  }
  return 0.0;
}

int db_get_sleep_history(sqlite3 *db, double *out_hours, int max_count) {
  sqlite3_stmt *stmt;
  const char *sql = "SELECT hours FROM ("
                    "  SELECT hours, logged_at, id FROM sleep_log "
                    "  ORDER BY logged_at DESC, id DESC LIMIT ?"
                    ") ORDER BY logged_at ASC, id ASC;";

  int count = 0;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    sqlite3_bind_int(stmt, 1, max_count);
    while (sqlite3_step(stmt) == SQLITE_ROW && count < max_count) {
      if (out_hours)
        out_hours[count] = sqlite3_column_double(stmt, 0);
      count++;
    }
    sqlite3_finalize(stmt);
  }
  return count;
}

double db_get_sleep_7d_avg(sqlite3 *db) {
  sqlite3_stmt *stmt;
  double avg = 0.0;
  const char *sql =
      "SELECT AVG(hours) FROM sleep_log "
      "WHERE logged_at >= datetime('now', '-7 days', 'localtime');";

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      avg = sqlite3_column_double(stmt, 0);
    }
    sqlite3_finalize(stmt);
  }
  return avg;
}

void db_get_today_nutrition(sqlite3 *db, double *out_calories,
                            double *out_protein, double *out_carbs,
                            double *out_fat) {
  sqlite3_stmt *stmt;
  const char *sql =
      "SELECT SUM(calories), SUM(protein_g), SUM(carbs_g), SUM(fat_g) FROM "
      "nutrition_log "
      "WHERE logged_at >= datetime('now', 'start of day', 'localtime');";

  if (out_calories)
    *out_calories = 0.0;
  if (out_protein)
    *out_protein = 0.0;
  if (out_carbs)
    *out_carbs = 0.0;
  if (out_fat)
    *out_fat = 0.0;

  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      if (out_calories)
        *out_calories = sqlite3_column_double(stmt, 0);
      if (out_protein)
        *out_protein = sqlite3_column_double(stmt, 1);
      if (out_carbs)
        *out_carbs = sqlite3_column_double(stmt, 2);
      if (out_fat)
        *out_fat = sqlite3_column_double(stmt, 3);
    }
    sqlite3_finalize(stmt);
  }
}

bool db_get_last_trained(sqlite3 *db, int *out_days_since, char *out_last_day,
                         int max_day_len) {
  sqlite3_stmt *stmt;
  /* Query the most recent logged workout */
  const char *sql = "SELECT "
                    "  CAST(julianday('now', 'localtime') - "
                    "julianday(logged_at) AS INTEGER) AS days_since, "
                    "  logged_at, "
                    "  exercise "
                    "FROM workouts "
                    "ORDER BY logged_at DESC, id DESC LIMIT 1;";

  bool found = false;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      int days = sqlite3_column_int(stmt, 0);
      const char *date = (const char *)sqlite3_column_text(stmt, 1);
      const char *ex = (const char *)sqlite3_column_text(stmt, 2);

      if (out_days_since)
        *out_days_since = days;

      if (out_last_day && date && ex) {
        /* Format e.g., "tuesday — bench press" */
        /* First get day of the week from the date */
        sqlite3_stmt *stmt_day;
        const char *day_sql = "SELECT strftime('%w', ?);";
        int wday = 0;

        if (sqlite3_prepare_v2(db, day_sql, -1, &stmt_day, NULL) == SQLITE_OK) {
          sqlite3_bind_text(stmt_day, 1, date, -1, SQLITE_STATIC);
          if (sqlite3_step(stmt_day) == SQLITE_ROW) {
            wday = sqlite3_column_int(stmt_day, 0);
          }
          sqlite3_finalize(stmt_day);
        }

        const char *days_names[] = {"sunday",    "monday",   "tuesday",
                                    "wednesday", "thursday", "friday",
                                    "saturday"};

        snprintf(out_last_day, max_day_len, "%s — %s", days_names[wday], ex);
      }
      found = true;
    }
    sqlite3_finalize(stmt);
  }
  return found;
}