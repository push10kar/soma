#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "db.h"
#include "utils.h"
#include "workout.h"
#include "suggest.h"
#include "status.h"

int main(int argc, char *argv[]) {

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

    /* Route: soma workout log --exercise ... --sets ... --reps ... --weight ... */
    if (argc >= 3 && strcmp(argv[1], "workout") == 0 && strcmp(argv[2], "log") == 0) {
        fprintf(stderr, "DEBUG: Parsing workout arguments (argc=%d)\n", argc - 2);

        /* Parse workout arguments starting from argv[3] */
        Workout* w = parse_workout_args(argc - 2, argv + 2);
        
        fprintf(stderr, "DEBUG: Parsed workout, checking errors\n");
        if (!w) {
            fprintf(stderr, "error: failed to parse workout arguments\n");
            return 1;
        }
        
        /* Check for parsing errors */
        if (w->error != WORKOUT_OK) {
            fprintf(stderr, "error: %s\n", w->error_msg ? w->error_msg : "unknown error");
            free_workout(w);
            return 1;
        }
        
        fprintf(stderr, "DEBUG: Logging workout to database\n");
        /* Log the workout to database */
        if (!log_workout(w)) {
            fprintf(stderr, "error: failed to log workout to database\n");
            free_workout(w);
            return 1;
        }
        
        fprintf(stderr, "DEBUG: Workout logged successfully\n");
        /* Print success confirmation */
        printf(POSITIVE "✓ " RESET "Logged " ACCENT BOLD "%s" RESET ": %d sets, " 
               ACCENT "%.0f kg-reps" RESET "\n", 
               w->exercise, w->num_sets, w->volume_kg);
        
        /* Print PR badge if new personal record */
        if (w->is_new_pr) {
            double max_weight = get_max_weight(w->weights);
            if (max_weight > 0) {
                printf("\n");
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
    fprintf(stderr, "error: command '%s' not recognized.\n\n", argv[1]);
    fprintf(stderr, "Available commands:\n");
    fprintf(stderr, "  soma today                    Shows compact daily briefing\n");
    fprintf(stderr, "  soma status                   Shows all ledger sections at once\n");
    fprintf(stderr, "  soma physique                 Shows physique analytics (weight, waist trends)\n");
    fprintf(stderr, "  soma recovery                 Shows recovery analytics (sleep rolling average)\n");
    fprintf(stderr, "  soma nutrition                Shows macro progress bars\n");
    fprintf(stderr, "  soma suggest [exercise]       Shows target weights and coaching advice\n");
    fprintf(stderr, "  soma history <exercise>       Shows history and volume trends (e.g. bench, squat, ohp)\n");
    fprintf(stderr, "\nLogging commands:\n");
    fprintf(stderr, "  soma workout log ...          Log a workout session\n");
    fprintf(stderr, "  soma bodyweight log ...       Log daily bodyweight and waist\n");
    fprintf(stderr, "  soma sleep log ...            Log daily sleep hours and quality\n");
    fprintf(stderr, "  soma nutrition log ...        Log daily macro and calorie intake\n");
    
    db_close(db);
    return 1;
}
