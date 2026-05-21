#ifndef DB_H
#define DB_H

#include <sqlite3.h>
#include <stdbool.h>

sqlite3 *db_open(void);
void db_close(sqlite3 *db);
void db_init(sqlite3 *db);

/* Seeding Engine */
void db_seed(sqlite3 *db);

/* Data Binding Queries */
int db_get_exercise_history(sqlite3 *db, const char *exercise, double *out_weights, int *out_reps, double *out_volumes, int max_count);
bool db_get_latest_workout_set(sqlite3 *db, const char *exercise, double *out_weight, int *out_reps);
double db_get_estimated_1rm(sqlite3 *db, const char *exercise);

int db_get_bodyweight_history(sqlite3 *db, double *out_weights, double *out_waist, int max_count);
double db_get_bodyweight_weekly_change(sqlite3 *db);
double db_get_waist_weekly_change(sqlite3 *db);

int db_get_sleep_history(sqlite3 *db, double *out_hours, int max_count);
double db_get_sleep_7d_avg(sqlite3 *db);

void db_get_today_nutrition(sqlite3 *db, double *out_calories, double *out_protein, double *out_carbs, double *out_fat);
bool db_get_last_trained(sqlite3 *db, int *out_days_since, char *out_last_day, int max_day_len);

/* Telemetry Writing Operations */
bool db_log_bodyweight(sqlite3 *db, double weight_kg, double waist_cm);
bool db_log_sleep(sqlite3 *db, double hours, int quality);
bool db_log_nutrition(sqlite3 *db, const char *meal, double calories, double protein, double carbs, double fat);

#endif