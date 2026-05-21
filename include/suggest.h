#ifndef SUGGEST_H
#define SUGGEST_H

#include <sqlite3.h>

/**
 * Suggestion module - calculates 1RM changes over the last 30 days
 * and recommends precise weight and rep targets for upcoming sessions.
 */

/* Computes 30-day 1RM changes and prints a premium workload suggestion dashboard */
void print_suggestions(sqlite3 *db, const char *filter_exercise);

#endif /* SUGGEST_H */
