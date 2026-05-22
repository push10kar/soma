#ifndef STATUS_H
#define STATUS_H

#include <sqlite3.h>

/**
 * Status and Dashboard Display Module
 * Controls all CLI subcommand visualization formatting.
 */

/* Renders the compact daily briefing (logo, trained day, sleep/weight status, daily recommendation) */
void print_today_briefing(sqlite3 *db);

/* Renders the full health ledger (Training, Physique, Recovery, Nutrition) */
void print_full_status(sqlite3 *db);

/* Renders only the physique metrics (bodyweight, waist tracking, sparklines, changes) */
void print_physique_dashboard(sqlite3 *db);

/* Renders only the recovery metrics (sleep average, readiness) */
void print_recovery_dashboard(sqlite3 *db);

/* Renders only the nutrition metrics (macro progress bars) */
void print_nutrition_dashboard(sqlite3 *db);

/* Renders dynamic workout log history and volume trends for a given exercise */
void print_exercise_history(sqlite3 *db, const char *exercise);

/* Renders the Sunday weekly review dashboard */
void print_weekly_review(sqlite3 *db);

#endif /* STATUS_H */
