#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "split_setup.h"
#include "utils.h"
#include "db.h"

#define MAX_SPLIT_DAYS 7
#define MAX_EXERCISES_PER_DAY 12

typedef struct {
    char name[64];
    int weekday; // 1-7 (1=Monday, 7=Sunday), or -1 for rotating
    char exercises[MAX_EXERCISES_PER_DAY][64];
    int exercise_count;
} TempSplitDay;

static void read_line(char *buf, size_t size) {
    if (!fgets(buf, size, stdin)) {
        buf[0] = '\0';
        return;
    }
    size_t len = strlen(buf);
    while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r' || buf[len - 1] == ' ')) {
        buf[len - 1] = '\0';
        len--;
    }
    char *p = buf;
    while (*p == ' ') p++;
    if (p != buf) {
        memmove(buf, p, strlen(p) + 1);
    }
}

bool is_split_configured(sqlite3 *db) {
    sqlite3_stmt *stmt;
    bool configured = false;
    const char *sql = "SELECT value FROM config WHERE key = 'split_type';";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *val = (const char *)sqlite3_column_text(stmt, 0);
            if (val && strlen(val) > 0) {
                configured = true;
            }
        }
        sqlite3_finalize(stmt);
    }
    return configured;
}

void run_split_setup_wizard(sqlite3 *db) {
    print_logo();
    print_separator();
    printf("  " ACCENT BOLD "soma" RESET "  ·  let's set up your training split\n\n");
    printf(DIM "  This takes about 2 minutes. You can redo this\n");
    printf("  anytime with: " RESET "soma split setup" DIM "\n\n");
    print_thin_sep();

    char buf[128];
    int total_days = 3;
    bool custom_days = false;

    while (1) {
        printf("  How many days is your split?\n\n");
        printf("    1.  3 days  (PPL, Push/Pull/Legs)\n");
        printf("    2.  4 days  (Upper/Lower x2)\n");
        printf("    3.  5 days  (custom)\n");
        printf("    4.  6 days  (PPL x2)\n");
        printf("    5.  custom  (I'll define my own)\n\n");
        printf("  ❯ ");
        fflush(stdout);
        read_line(buf, sizeof(buf));

        if (strcmp(buf, "1") == 0) {
            total_days = 3;
            break;
        } else if (strcmp(buf, "2") == 0) {
            total_days = 4;
            break;
        } else if (strcmp(buf, "3") == 0) {
            total_days = 5;
            break;
        } else if (strcmp(buf, "4") == 0) {
            total_days = 6;
            break;
        } else if (strcmp(buf, "5") == 0) {
            custom_days = true;
            break;
        } else {
            printf(WARNING "  Invalid option. Please enter 1-5." RESET "\n\n");
        }
    }

    if (custom_days) {
        while (1) {
            printf("\n  Enter number of days (1-7): ");
            fflush(stdout);
            read_line(buf, sizeof(buf));
            int val = atoi(buf);
            if (val >= 1 && val <= 7) {
                total_days = val;
                break;
            }
            printf(WARNING "  Please enter a number between 1 and 7." RESET "\n");
        }
    }

    bool is_rotating = true;
    while (1) {
        printf("\n  Do you train on fixed weekdays or rotate?\n\n");
        printf("    1.  fixed    (push always Monday/Thursday)\n");
        printf("    2.  rotate   (cycle through, any day)\n\n");
        printf("  ❯ ");
        fflush(stdout);
        read_line(buf, sizeof(buf));

        if (strcmp(buf, "1") == 0) {
            is_rotating = false;
            break;
        } else if (strcmp(buf, "2") == 0) {
            is_rotating = true;
            break;
        } else {
            printf(WARNING "  Invalid option. Please enter 1 or 2." RESET "\n");
        }
    }

    TempSplitDay split[MAX_SPLIT_DAYS];
    memset(split, 0, sizeof(split));

    const char *weekday_names[] = {
        "", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"
    };

    for (int d = 0; d < total_days; d++) {
        split[d].weekday = -1;
        printf("\n" BOLD "  Day %d — what do you call this day?" RESET "\n", d + 1);
        printf(DIM "  (e.g. push, pull, legs, upper, lower, chest, back)" RESET "\n\n  ❯ ");
        fflush(stdout);
        read_line(split[d].name, sizeof(split[d].name));

        if (strlen(split[d].name) == 0) {
            snprintf(split[d].name, sizeof(split[d].name), "Day %d", d + 1);
        }

        if (!is_rotating) {
            while (1) {
                printf("  Which weekday do you train %s on? (1-7, where 1=Mon, 7=Sun): ", split[d].name);
                fflush(stdout);
                read_line(buf, sizeof(buf));
                int wd = atoi(buf);
                if (wd >= 1 && wd <= 7) {
                    split[d].weekday = wd;
                    break;
                }
                printf(WARNING "  Please enter a weekday number between 1 and 7." RESET "\n");
            }
        }

        printf("\n  What are your main exercises on %s day?\n", split[d].name);
        printf(DIM "  Enter one per line. Empty line when done." RESET "\n\n");

        int ex_idx = 0;
        while (ex_idx < MAX_EXERCISES_PER_DAY) {
            printf("  ❯ ");
            fflush(stdout);
            read_line(split[d].exercises[ex_idx], sizeof(split[d].exercises[ex_idx]));
            if (strlen(split[d].exercises[ex_idx]) == 0) {
                break;
            }
            ex_idx++;
        }
        split[d].exercise_count = ex_idx;
        printf("\n" POSITIVE "  Got it. %s day — %d exercises." RESET "\n", split[d].name, ex_idx);
    }

    print_separator();
    printf("  " BOLD "Your split" RESET "\n\n");

    for (int d = 0; d < total_days; d++) {
        char ex_list[512] = "";
        for (int i = 0; i < split[d].exercise_count; i++) {
            strcat(ex_list, split[d].exercises[i]);
            if (i < split[d].exercise_count - 1) {
                strcat(ex_list, ", ");
            }
        }
        if (strlen(ex_list) == 0) {
            strcpy(ex_list, "rest / active recovery");
        }

        if (is_rotating) {
            printf("  Day %d  %-10s  %s\n", d + 1, split[d].name, ex_list);
        } else {
            printf("  Day %d  %-10s  %-10s  %s\n", d + 1, split[d].name, weekday_names[split[d].weekday], ex_list);
        }
    }

    printf("\n  Schedule: " ACCENT "%s" RESET "\n\n", is_rotating ? "rotating" : "fixed");

    while (1) {
        printf("  Looks good? (y/n) ❯ ");
        fflush(stdout);
        read_line(buf, sizeof(buf));

        if (strcmp(buf, "y") == 0 || strcmp(buf, "Y") == 0) {
            break;
        } else if (strcmp(buf, "n") == 0 || strcmp(buf, "N") == 0) {
            printf("\n" WARNING "  Aborting setup. Your training split was not saved." RESET "\n");
            print_separator();
            return;
        } else {
            printf(WARNING "  Please enter y or n." RESET "\n");
        }
    }

    /* Save to Database Transactionally */
    char *err_msg = NULL;
    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    // Clear old split config
    sqlite3_exec(db, "DELETE FROM config WHERE key IN ('split_type', 'split_rotation_pos', 'split_days_count');", NULL, NULL, NULL);
    sqlite3_exec(db, "DELETE FROM split_days;", NULL, NULL, NULL);
    sqlite3_exec(db, "DELETE FROM split_exercises;", NULL, NULL, NULL);

    sqlite3_stmt *stmt;
    
    // Save general config
    const char *cfg_sql = "INSERT INTO config (key, value) VALUES (?, ?);";
    if (sqlite3_prepare_v2(db, cfg_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, "split_type", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, is_rotating ? "rotate" : "fixed", -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);

        sqlite3_bind_text(stmt, 1, "split_rotation_pos", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, "0", -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);

        char days_str[8];
        snprintf(days_str, sizeof(days_str), "%d", total_days);
        sqlite3_bind_text(stmt, 1, "split_days_count", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, days_str, -1, SQLITE_STATIC);
        sqlite3_step(stmt);

        sqlite3_finalize(stmt);
    }

    // Save split days and exercises
    const char *day_sql = "INSERT INTO split_days (id, name, weekday) VALUES (?, ?, ?);";
    const char *ex_sql = "INSERT INTO split_exercises (split_day_id, exercise) VALUES (?, ?);";

    sqlite3_stmt *stmt_day, *stmt_ex;
    if (sqlite3_prepare_v2(db, day_sql, -1, &stmt_day, NULL) == SQLITE_OK &&
        sqlite3_prepare_v2(db, ex_sql, -1, &stmt_ex, NULL) == SQLITE_OK) {
        
        for (int d = 0; d < total_days; d++) {
            sqlite3_bind_int(stmt_day, 1, d);
            sqlite3_bind_text(stmt_day, 2, split[d].name, -1, SQLITE_STATIC);
            if (is_rotating) {
                sqlite3_bind_null(stmt_day, 3);
            } else {
                sqlite3_bind_int(stmt_day, 3, split[d].weekday);
            }
            sqlite3_step(stmt_day);
            sqlite3_reset(stmt_day);

            for (int i = 0; i < split[d].exercise_count; i++) {
                sqlite3_bind_int(stmt_ex, 1, d);
                sqlite3_bind_text(stmt_ex, 2, split[d].exercises[i], -1, SQLITE_STATIC);
                sqlite3_step(stmt_ex);
                sqlite3_reset(stmt_ex);
            }
        }
        sqlite3_finalize(stmt_day);
        sqlite3_finalize(stmt_ex);
    }

    if (sqlite3_exec(db, "COMMIT;", NULL, NULL, &err_msg) == SQLITE_OK) {
        printf("\n" POSITIVE "✓ Training split configured and saved permanently!" RESET "\n");
    } else {
        fprintf(stderr, "error: transaction commit failed: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_exec(db, "ROLLBACK;", NULL, NULL, NULL);
    }

    print_separator();
}

void print_split_summary(sqlite3 *db) {
    print_logo();
    print_separator();

    sqlite3_stmt *stmt_cfg;
    char split_type[32] = "rotate";
    const char *sql_cfg = "SELECT value FROM config WHERE key = 'split_type';";
    if (sqlite3_prepare_v2(db, sql_cfg, -1, &stmt_cfg, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt_cfg) == SQLITE_ROW) {
            const char *val = (const char *)sqlite3_column_text(stmt_cfg, 0);
            if (val) strncpy(split_type, val, sizeof(split_type));
        }
        sqlite3_finalize(stmt_cfg);
    }

    int total_days = 0;
    const char *sql_count = "SELECT COUNT(*) FROM split_days;";
    if (sqlite3_prepare_v2(db, sql_count, -1, &stmt_cfg, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt_cfg) == SQLITE_ROW) {
            total_days = sqlite3_column_int(stmt_cfg, 0);
        }
        sqlite3_finalize(stmt_cfg);
    }

    int pos = 0;
    if (strcmp(split_type, "rotate") == 0) {
        const char *sql_pos = "SELECT value FROM config WHERE key = 'split_rotation_pos';";
        if (sqlite3_prepare_v2(db, sql_pos, -1, &stmt_cfg, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt_cfg) == SQLITE_ROW) {
                const char *val = (const char *)sqlite3_column_text(stmt_cfg, 0);
                if (val) pos = atoi(val);
            }
            sqlite3_finalize(stmt_cfg);
        }
    }

    int user_wd = 1;
    if (strcmp(split_type, "fixed") == 0) {
        sqlite3_stmt *stmt_wd;
        int sqlite_wd = 1;
        if (sqlite3_prepare_v2(db, "SELECT strftime('%w', 'now', 'localtime');", -1, &stmt_wd, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt_wd) == SQLITE_ROW) {
                const char *val = (const char *)sqlite3_column_text(stmt_wd, 0);
                if (val) sqlite_wd = atoi(val);
            }
            sqlite3_finalize(stmt_wd);
        }
        user_wd = (sqlite_wd == 0) ? 7 : sqlite_wd;
    }

    printf("  " ACCENT BOLD "your split" RESET "  ·  " BOLD "%d day %s" RESET "\n\n", total_days, split_type);

    const char *months_names[] = {
        "jan", "feb", "mar", "apr", "may", "jun",
        "jul", "aug", "sep", "oct", "nov", "dec"
    };

    sqlite3_stmt *stmt_day;
    const char *sql_day = "SELECT id, name, weekday FROM split_days ORDER BY id ASC;";
    if (sqlite3_prepare_v2(db, sql_day, -1, &stmt_day, NULL) == SQLITE_OK) {
        while (sqlite3_step(stmt_day) == SQLITE_ROW) {
            int d_id = sqlite3_column_int(stmt_day, 0);
            const char *name = (const char *)sqlite3_column_text(stmt_day, 1);
            int weekday = sqlite3_column_type(stmt_day, 2) == SQLITE_NULL ? -1 : sqlite3_column_int(stmt_day, 2);

            // Fetch last trained date
            char last_str[64] = "never";
            sqlite3_stmt *stmt_last;
            const char *sql_last = 
                "SELECT max(logged_at) FROM workouts "
                "WHERE LOWER(exercise) IN ( "
                "    SELECT LOWER(exercise) FROM split_exercises WHERE split_day_id = ? "
                ");";
            if (sqlite3_prepare_v2(db, sql_last, -1, &stmt_last, NULL) == SQLITE_OK) {
                sqlite3_bind_int(stmt_last, 1, d_id);
                if (sqlite3_step(stmt_last) == SQLITE_ROW) {
                    const char *max_date = (const char *)sqlite3_column_text(stmt_last, 0);
                    if (max_date) {
                        // Calculate days diff
                        int days_diff = -1;
                        sqlite3_stmt *stmt_diff;
                        const char *sql_diff = "SELECT CAST(julianday('now', 'localtime') - julianday(?) AS INTEGER);";
                        if (sqlite3_prepare_v2(db, sql_diff, -1, &stmt_diff, NULL) == SQLITE_OK) {
                            sqlite3_bind_text(stmt_diff, 1, max_date, -1, SQLITE_STATIC);
                            if (sqlite3_step(stmt_diff) == SQLITE_ROW) {
                                days_diff = sqlite3_column_int(stmt_diff, 0);
                            }
                            sqlite3_finalize(stmt_diff);
                        }

                        if (days_diff == 0) {
                            strcpy(last_str, "today");
                        } else if (days_diff == 1) {
                            strcpy(last_str, "yesterday");
                        } else {
                            // Extract month/day
                            sqlite3_stmt *stmt_fmt;
                            if (sqlite3_prepare_v2(db, "SELECT strftime('%m-%d', ?);", -1, &stmt_fmt, NULL) == SQLITE_OK) {
                                sqlite3_bind_text(stmt_fmt, 1, max_date, -1, SQLITE_STATIC);
                                if (sqlite3_step(stmt_fmt) == SQLITE_ROW) {
                                    const char *md = (const char *)sqlite3_column_text(stmt_fmt, 0);
                                    if (md) {
                                        int m = 0, d = 0;
                                        if (sscanf(md, "%d-%d", &m, &d) == 2) {
                                            snprintf(last_str, sizeof(last_str), "%s %d", months_names[m - 1], d);
                                        }
                                    }
                                }
                                sqlite3_finalize(stmt_fmt);
                            }
                        }
                    }
                }
                sqlite3_finalize(stmt_last);
            }

            // Calculate next due date
            int diff = 0;
            if (strcmp(split_type, "rotate") == 0) {
                diff = (d_id - pos + total_days) % total_days;
            } else {
                diff = (weekday - user_wd + 7) % 7;
            }

            char next_str[64] = "today";
            if (diff == 0) {
                strcpy(next_str, "today");
            } else if (diff == 1) {
                strcpy(next_str, "tomorrow");
            } else {
                sqlite3_stmt *stmt_next;
                const char *sql_next = "SELECT strftime('%m-%d', 'now', 'localtime', '+' || ? || ' days');";
                if (sqlite3_prepare_v2(db, sql_next, -1, &stmt_next, NULL) == SQLITE_OK) {
                    sqlite3_bind_int(stmt_next, 1, diff);
                    if (sqlite3_step(stmt_next) == SQLITE_ROW) {
                        const char *md = (const char *)sqlite3_column_text(stmt_next, 0);
                        if (md) {
                            int m = 0, d = 0;
                            if (sscanf(md, "%d-%d", &m, &d) == 2) {
                                snprintf(next_str, sizeof(next_str), "%s %d", months_names[m - 1], d);
                            }
                        }
                    }
                    sqlite3_finalize(stmt_next);
                }
            }

            // Print beautifully
            char last_padded[64];
            snprintf(last_padded, sizeof(last_padded), "%-10s", last_str);

            char last_display[128];
            if (strcmp(last_str, "never") == 0) {
                snprintf(last_display, sizeof(last_display), DIM "%s" RESET, last_padded);
            } else if (strcmp(last_str, "today") == 0 || strcmp(last_str, "yesterday") == 0) {
                snprintf(last_display, sizeof(last_display), ACCENT "%s" RESET, last_padded);
            } else {
                snprintf(last_display, sizeof(last_display), "%s", last_padded);
            }

            char next_padded[64];
            snprintf(next_padded, sizeof(next_padded), "%-10s", next_str);

            char next_display[128];
            if (strcmp(next_str, "today") == 0) {
                snprintf(next_display, sizeof(next_display), POSITIVE "%s" RESET, next_padded);
            } else {
                snprintf(next_display, sizeof(next_display), "%s", next_padded);
            }

            printf("  day %-2d  %-8s  last %s  ·  next %s\n", 
                   d_id + 1, name, last_display, next_display);
        }
        sqlite3_finalize(stmt_day);
    }

    printf("\n");

    // Print active position
    ActiveSplitDay active = get_active_split_day(db);
    if (active.is_rest_day) {
        printf("  %-12s" DIM "%s" RESET "\n", "position", "rest / active recovery");
    } else {
        printf("  %-12sday %d of %d  (%s today)\n", 
               "position", active.id + 1, total_days, active.name);
    }

    print_separator();
}

static const char* resolve_exercise_name_local(const char* name) {
    if (strcasecmp(name, "bench") == 0 || strcasecmp(name, "bp") == 0 || strcasecmp(name, "bench press") == 0) {
        return "bench press";
    }
    if (strcasecmp(name, "squat") == 0 || strcasecmp(name, "sq") == 0) {
        return "squat";
    }
    if (strcasecmp(name, "ohp") == 0 || strcasecmp(name, "overhead") == 0 || strcasecmp(name, "overhead press") == 0 || strcasecmp(name, "press") == 0) {
        return "overhead press";
    }
    if (strcasecmp(name, "deadlift") == 0 || strcasecmp(name, "dl") == 0) {
        return "deadlift";
    }
    return name;
}

ActiveSplitDay get_active_split_day(sqlite3 *db) {
    ActiveSplitDay active;
    active.id = -1;
    strcpy(active.name, "rest");
    active.is_rest_day = true;

    sqlite3_stmt *stmt;
    char split_type[32] = "";
    
    // Get split type
    const char *sql_cfg = "SELECT value FROM config WHERE key = 'split_type';";
    if (sqlite3_prepare_v2(db, sql_cfg, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *val = (const char *)sqlite3_column_text(stmt, 0);
            if (val) strncpy(split_type, val, sizeof(split_type));
        }
        sqlite3_finalize(stmt);
    }

    if (strcmp(split_type, "rotate") == 0) {
        int pos = 0;
        const char *sql_pos = "SELECT value FROM config WHERE key = 'split_rotation_pos';";
        if (sqlite3_prepare_v2(db, sql_pos, -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char *val = (const char *)sqlite3_column_text(stmt, 0);
                if (val) pos = atoi(val);
            }
            sqlite3_finalize(stmt);
        }

        const char *sql_day = "SELECT id, name FROM split_days WHERE id = ?;";
        if (sqlite3_prepare_v2(db, sql_day, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, pos);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                active.id = sqlite3_column_int(stmt, 0);
                const char *name = (const char *)sqlite3_column_text(stmt, 1);
                if (name) strncpy(active.name, name, sizeof(active.name));
                active.is_rest_day = false;
            }
            sqlite3_finalize(stmt);
        }
    } else if (strcmp(split_type, "fixed") == 0) {
        int sqlite_wd = 1;
        if (sqlite3_prepare_v2(db, "SELECT strftime('%w', 'now', 'localtime');", -1, &stmt, NULL) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                const char *val = (const char *)sqlite3_column_text(stmt, 0);
                if (val) sqlite_wd = atoi(val);
            }
            sqlite3_finalize(stmt);
        }
        int user_wd = (sqlite_wd == 0) ? 7 : sqlite_wd;

        const char *sql_day = "SELECT id, name FROM split_days WHERE weekday = ?;";
        if (sqlite3_prepare_v2(db, sql_day, -1, &stmt, NULL) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, user_wd);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                active.id = sqlite3_column_int(stmt, 0);
                const char *name = (const char *)sqlite3_column_text(stmt, 1);
                if (name) strncpy(active.name, name, sizeof(active.name));
                active.is_rest_day = false;
            }
            sqlite3_finalize(stmt);
        }
    }

    return active;
}

bool is_exercise_in_split_day(sqlite3 *db, int split_day_id, const char *exercise) {
    if (split_day_id < 0) return false;

    sqlite3_stmt *stmt;
    bool found = false;
    const char *sql = "SELECT exercise FROM split_exercises WHERE split_day_id = ?;";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, split_day_id);
        const char *canonical_input = resolve_exercise_name_local(exercise);

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *split_ex = (const char *)sqlite3_column_text(stmt, 0);
            if (split_ex) {
                const char *canonical_split = resolve_exercise_name_local(split_ex);
                if (strcasecmp(canonical_input, canonical_split) == 0) {
                    found = true;
                    break;
                }
            }
        }
        sqlite3_finalize(stmt);
    }
    return found;
}

void advance_split_rotation_if_needed(sqlite3 *db) {
    sqlite3_stmt *stmt;
    char split_type[32] = "";
    
    // Get split type
    const char *sql_cfg = "SELECT value FROM config WHERE key = 'split_type';";
    if (sqlite3_prepare_v2(db, sql_cfg, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *val = (const char *)sqlite3_column_text(stmt, 0);
            if (val) strncpy(split_type, val, sizeof(split_type));
        }
        sqlite3_finalize(stmt);
    }

    if (strcmp(split_type, "rotate") != 0) {
        return; // Fixed split does not advance
    }

    int pos = 0;
    const char *sql_pos = "SELECT value FROM config WHERE key = 'split_rotation_pos';";
    if (sqlite3_prepare_v2(db, sql_pos, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *val = (const char *)sqlite3_column_text(stmt, 0);
            if (val) pos = atoi(val);
        }
        sqlite3_finalize(stmt);
    }

    int days_count = 3;
    const char *sql_count = "SELECT value FROM config WHERE key = 'split_days_count';";
    if (sqlite3_prepare_v2(db, sql_count, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *val = (const char *)sqlite3_column_text(stmt, 0);
            if (val) days_count = atoi(val);
        }
        sqlite3_finalize(stmt);
    }
    if (days_count <= 0) days_count = 3;

    char last_advance_date[32] = "";
    const char *sql_date = "SELECT value FROM config WHERE key = 'last_rotation_advance_date';";
    if (sqlite3_prepare_v2(db, sql_date, -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *val = (const char *)sqlite3_column_text(stmt, 0);
            if (val) strncpy(last_advance_date, val, sizeof(last_advance_date));
        }
        sqlite3_finalize(stmt);
    }

    char today_date[32] = "";
    if (sqlite3_prepare_v2(db, "SELECT date('now', 'localtime');", -1, &stmt, NULL) == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *val = (const char *)sqlite3_column_text(stmt, 0);
            if (val) strncpy(today_date, val, sizeof(today_date));
        }
        sqlite3_finalize(stmt);
    }

    if (strcmp(last_advance_date, today_date) == 0) {
        return; // Already advanced today!
    }

    // Advance rotation pos
    int next_pos = (pos + 1) % days_count;
    char next_pos_str[32];
    snprintf(next_pos_str, sizeof(next_pos_str), "%d", next_pos);

    sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, NULL);

    const char *update_sql = "INSERT OR REPLACE INTO config (key, value) VALUES (?, ?);";
    if (sqlite3_prepare_v2(db, update_sql, -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, "split_rotation_pos", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, next_pos_str, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);

        sqlite3_bind_text(stmt, 1, "last_rotation_advance_date", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, today_date, -1, SQLITE_STATIC);
        sqlite3_step(stmt);

        sqlite3_finalize(stmt);
    }

    sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL);
}
