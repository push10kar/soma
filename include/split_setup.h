#ifndef SPLIT_SETUP_H
#define SPLIT_SETUP_H

#include <sqlite3.h>
#include <stdbool.h>

/**
 * Checks if the training split has been configured.
 * Returns true if split_type is set in the config table.
 */
bool is_split_configured(sqlite3 *db);

/**
 * Runs the interactive terminal-based training split questionnaire.
 * Stores all split data, days, and exercises in sqlite.
 */
void run_split_setup_wizard(sqlite3 *db);

/**
 * Renders a visual summary of the configured split.
 */
void print_split_summary(sqlite3 *db);

/**
 * Struct representing today's split day info.
 */
typedef struct {
    int id;               // 0-indexed split day id
    char name[64];        // e.g. "push"
    bool is_rest_day;     // true if no training scheduled today
} ActiveSplitDay;

/**
 * Gets the current active split day based on split schedule.
 * If rotating, returns the day matching split_rotation_pos.
 * If fixed, returns the day matching the current weekday.
 */
ActiveSplitDay get_active_split_day(sqlite3 *db);

/**
 * Checks if a given exercise name belongs to the active split day.
 * Returns true if it matches.
 */
bool is_exercise_in_split_day(sqlite3 *db, int split_day_id, const char *exercise);

/**
 * Advances the rotation counter by one if not already advanced today.
 * Should be called when logging a workout exercise.
 */
void advance_split_rotation_if_needed(sqlite3 *db);

#endif /* SPLIT_SETUP_H */
