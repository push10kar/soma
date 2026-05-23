#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"
#include "utils.h"
#include "workout.h"
#include "suggest.h"
#include "status.h"
#include "split_setup.h"

static void print_help(void) {
    printf("\n" ACCENT BOLD "  soma" RESET DIM "  ·  personal fitness ledger CLI" RESET "\n\n");
    printf("  Usage: soma <command> [args]\n\n");
    
    printf(ACCENT "  ▸ display commands" RESET "\n");
    printf("    %-38s%s\n", "soma [today]", "Show today's status & training splits");
    printf("    %-38s%s\n", "soma status", "Full ledger of training & health stats");
    printf("    %-38s%s\n", "soma weekly", "Sunday weekly overview & PR review");
    printf("    %-38s%s\n", "soma streak", "Checklist & consistency heatmap dashboard");
    printf("    %-38s%s\n", "soma prs", "List personal records & estimated 1RMs");
    printf("    %-38s%s\n", "soma physique", "Bodyweight & waist velocity analytics");
    printf("    %-38s%s\n", "soma split [setup]", "Set up or view training split schedule");
    printf("    %-38s%s\n", "soma recovery", "Sleep stats & CNS readiness score");
    printf("    %-38s%s\n", "soma nutrition", "Track today's macros vs target goals");
    printf("    %-38s%s\n", "soma suggest [exercise]", "Get targeted 30d progression weight");
    printf("    %-38s%s\n", "soma history <exercise>", "Log history & volume trend sparklines");
    printf("    %-38s%s\n\n", "soma heatmap", "GitHub-style yearly consistency grid");

    printf(ACCENT "  ▸ logging commands" RESET "\n");
    printf("    %-38s%s\n", "soma bodyweight log <w> [wt]", "Log daily weight and waist size");
    printf("    %-38s%s\n", "soma sleep log <hrs> <ql>", "Log sleep hours and quality (1-5)");
    printf("    %-38s%s\n", "soma nutrition log <c> <p> <cr> <f>", "Log daily calories & macros");
    printf("    %-38s%s\n", "soma log <ex> <w>x<r>x<s>", "Shorthand logging (e.g. bench 80x5x3)");
    printf("    %-38s%s\n\n", "soma workout log ...", "Full workout logging with parameters");
}

int main(int argc, char *argv[]) {

    /* Route: help / --help / -h */
    if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "help") == 0)) {
        print_help();
        return 0;
    }

    /* Route: soma bodyweight log <weight> [waist] */
    if (argc >= 3 && strcmp(argv[1], "bodyweight") == 0 && strcmp(argv[2], "log") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Usage: soma bodyweight log <weight_kg> [waist_cm]\n");
            return 1;
        }
        double weight = atof(argv[3]);
        double waist = (argc >= 5) ? atof(argv[4]) : -1.0;
        if (weight <= 0.0) {
            fprintf(stderr, "error: weight must be greater than 0\n");
            return 1;
        }
        sqlite3 *db = db_open();
        if (db_log_bodyweight(db, weight, waist)) {
            if (waist > 0.0) {
                printf(POSITIVE "✓ " RESET "Logged bodyweight: " ACCENT BOLD "%.1f kg" RESET ", waist: " ACCENT BOLD "%.1f cm" RESET "\n", weight, waist);
            } else {
                printf(POSITIVE "✓ " RESET "Logged bodyweight: " ACCENT BOLD "%.1f kg" RESET "\n", weight);
            }
            db_close(db);
            return 0;
        } else {
            fprintf(stderr, "error: failed to log bodyweight\n");
            db_close(db);
            return 1;
        }
    }

    /* Route: soma sleep log <hours> <quality> */
    if (argc >= 3 && strcmp(argv[1], "sleep") == 0 && strcmp(argv[2], "log") == 0) {
        if (argc < 5) {
            fprintf(stderr, "Usage: soma sleep log <hours> <quality_1_to_5>\n");
            return 1;
        }
        double hours = atof(argv[3]);
        int quality = atoi(argv[4]);
        if (hours < 0.0 || hours > 24.0) {
            fprintf(stderr, "error: sleep hours must be between 0 and 24\n");
            return 1;
        }
        if (quality < 1 || quality > 5) {
            fprintf(stderr, "error: sleep quality must be an integer between 1 and 5\n");
            return 1;
        }
        sqlite3 *db = db_open();
        if (db_log_sleep(db, hours, quality)) {
            printf(POSITIVE "✓ " RESET "Logged sleep: " ACCENT BOLD "%.1f hours" RESET " (quality: " ACCENT BOLD "%d/5" RESET ")\n", hours, quality);
            db_close(db);
            return 0;
        } else {
            fprintf(stderr, "error: failed to log sleep\n");
            db_close(db);
            return 1;
        }
    }

    /* Route: soma nutrition log <calories> <protein> <carbs> <fat> */
    if (argc >= 3 && strcmp(argv[1], "nutrition") == 0 && strcmp(argv[2], "log") == 0) {
        if (argc < 7) {
            fprintf(stderr, "Usage: soma nutrition log <calories> <protein_g> <carbs_g> <fat_g>\n");
            return 1;
        }
        double calories = atof(argv[3]);
        double protein = atof(argv[4]);
        double carbs = atof(argv[5]);
        double fat = atof(argv[6]);
        if (calories < 0.0 || protein < 0.0 || carbs < 0.0 || fat < 0.0) {
            fprintf(stderr, "error: nutrition values cannot be negative\n");
            return 1;
        }
        sqlite3 *db = db_open();
        if (db_log_nutrition(db, "CLI Log", calories, protein, carbs, fat)) {
            printf(POSITIVE "✓ " RESET "Logged nutrition: " ACCENT BOLD "%.0f kcal" RESET " (P: " ACCENT "%.0fg" RESET ", C: " ACCENT "%.0fg" RESET ", F: " ACCENT "%.0fg" RESET ")\n",
                   calories, protein, carbs, fat);
            db_close(db);
            return 0;
        } else {
            fprintf(stderr, "error: failed to log nutrition\n");
            db_close(db);
            return 1;
        }
    }

    /* Route: soma suggest [exercise] */
    if (argc >= 2 && strcmp(argv[1], "suggest") == 0) {
        const char *filter = (argc >= 3) ? argv[2] : NULL;
        sqlite3 *db = db_open();
        print_suggestions(db, filter);
        db_close(db);
        return 0;
    }

    /* Workout logging routing block (handles both shorthand and longhand) */
    bool is_workout_log = false;
    bool is_shorthand = false;
    int arg_offset = 0;
    
    if (argc >= 3 && strcmp(argv[1], "workout") == 0 && strcmp(argv[2], "log") == 0) {
        is_workout_log = true;
        arg_offset = 2; // skip "workout", "log"
        
        // Check if any argument starting from index 3 starts with "--"
        bool has_longhand_flag = false;
        for (int i = 3; i < argc; i++) {
            if (strncmp(argv[i], "--", 2) == 0) {
                has_longhand_flag = true;
                break;
            }
        }
        is_shorthand = !has_longhand_flag;
    } else if (argc >= 3 && strcmp(argv[1], "log") == 0) {
        is_workout_log = true;
        arg_offset = 1; // skip "log"
        is_shorthand = true;
    }
    
    if (is_workout_log) {
        Workout* w = NULL;
        if (is_shorthand) {
            w = parse_workout_shorthand(argc - (arg_offset + 1), argv + (arg_offset + 1));
        } else {
            w = parse_workout_args(argc - arg_offset, argv + arg_offset);
        }
        
        if (!w) {
            fprintf(stderr, "error: failed to parse workout arguments\n");
            return 1;
        }
        
        if (w->error != WORKOUT_OK) {
            fprintf(stderr, "error: %s\n", w->error_msg ? w->error_msg : "unknown error");
            free_workout(w);
            return 1;
        }
        
        if (!log_workout(w)) {
            fprintf(stderr, "error: failed to log workout to database\n");
            free_workout(w);
            return 1;
        }
        
        sqlite3 *db = db_open();
        if (db) {
            print_logged_workout_summary(db, w);
            db_close(db);
        }
        
        if (w->is_new_pr) {
            double max_weight = get_max_weight(w->weights);
            if (max_weight > 0) {
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

    /* Manual route: soma split setup / soma split */
    if (argc >= 2 && strcmp(argv[1], "split") == 0) {
        if (argc >= 3 && strcmp(argv[2], "setup") == 0) {
            run_split_setup_wizard(db);
            db_close(db);
            return 0;
        } else {
            if (is_split_configured(db)) {
                print_split_summary(db);
            } else {
                printf("\n" WARNING "  No training split is configured yet." RESET "\n");
                printf("  To set up your training split, run: " BOLD "soma split setup" RESET "\n\n");
            }
            db_close(db);
            return 0;
        }
    }

    /* Auto-trigger on first-run if split is not configured */
    if (!is_split_configured(db)) {
        run_split_setup_wizard(db);
        db_close(db);
        return 0;
    }

    if (argc == 1 || strcmp(argv[1], "today") == 0) {
        print_today_briefing(db);
        db_close(db);
        return 0;
    }
    
    if (strcmp(argv[1], "status") == 0) {
        print_full_status(db);
        db_close(db);
        return 0;
    }
    
    if (strcmp(argv[1], "weekly") == 0) {
        print_weekly_review(db);
        db_close(db);
        return 0;
    }
    
    if (strcmp(argv[1], "streak") == 0 || strcmp(argv[1], "streaks") == 0) {
        print_streak_dashboard(db);
        db_close(db);
        return 0;
    }
    
    if (strcmp(argv[1], "prs") == 0 || strcmp(argv[1], "pr") == 0) {
        print_prs_dashboard(db);
        db_close(db);
        return 0;
    }
    
    if (strcmp(argv[1], "physique") == 0 || strcmp(argv[1], "bodyweight") == 0) {
        print_physique_dashboard(db);
        db_close(db);
        return 0;
    }
    
    if (strcmp(argv[1], "recovery") == 0 || strcmp(argv[1], "sleep") == 0) {
        print_recovery_dashboard(db);
        db_close(db);
        return 0;
    }
    
    if (strcmp(argv[1], "nutrition") == 0) {
        print_nutrition_dashboard(db);
        db_close(db);
        return 0;
    }

    if (strcmp(argv[1], "heatmap") == 0 || strcmp(argv[1], "activity") == 0) {
        print_activity_heatmap(db);
        db_close(db);
        return 0;
    }

    if (strcmp(argv[1], "history") == 0 || strcmp(argv[1], "last") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: soma history <exercise_name> (e.g. bench, squat, ohp)\n");
            db_close(db);
            return 1;
        }
        print_exercise_history(db, argv[2]);
        db_close(db);
        return 0;
    }

    // Fallback: unrecognized subcommand
    fprintf(stderr, NEGATIVE "error: command '%s' not recognized." RESET "\n", argv[1]);
    print_help();
    
    db_close(db);
    return 1;
}
