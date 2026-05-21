#ifndef WORKOUT_H
#define WORKOUT_H

#include <stdbool.h>

/**
 * Workout logging module - handles exercise logging, volume calculation, and PR detection
 */

/* Error codes for workout operations */
typedef enum {
    WORKOUT_OK = 0,
    WORKOUT_ERR_INVALID_ARGS = 1,
    WORKOUT_ERR_PARSE_FAILED = 2,
    WORKOUT_ERR_VALIDATION_FAILED = 3,
    WORKOUT_ERR_DB_FAILED = 4,
    WORKOUT_ERR_MEMORY = 5,
} WorkoutError;

/* Represents a parsed workout session */
typedef struct {
    char* exercise;           /* Exercise name (e.g., "Bench Press") */
    char* reps;               /* Comma-separated reps (e.g., "8,8,7,6") */
    char* weights;            /* Comma-separated weights in kg (e.g., "100,100,95,90") */
    int num_sets;             /* Number of sets */
    double volume_kg;         /* Total volume: sum of (weight × reps) per set */
    bool is_new_pr;           /* True if this workout set a new personal record */
    WorkoutError error;       /* Error code if parsing/validation failed */
    char* error_msg;          /* Human-readable error message */
} Workout;

/* Temporary struct for parsing individual sets during validation */
typedef struct {
    double weight;
    int reps;
} WorkoutSet;

/**
 * Parse command-line arguments for workout logging
 * 
 * Expected format: --exercise NAME --sets N --reps "r1,r2,...,rN" --weight "w1,w2,...,wN"
 * 
 * Example: --exercise "Bench Press" --sets 4 --reps "8,8,7,6" --weight "100,100,95,90"
 * 
 * Returns: Workout struct with parsed data, or error set in the struct
 * Caller must call free_workout() to cleanup
 */
Workout* parse_workout_args(int argc, char* argv[]);

/**
 * Log a workout to the database and detect personal records
 * 
 * Performs:
 * 1. Validates the workout struct
 * 2. Queries personal_records table for current all-time max weight
 * 3. If new max weight found, inserts into personal_records with is_new_pr flag
 * 4. Inserts into workouts table
 * 
 * Returns: true on success, false on database error (check workout->error)
 */
bool log_workout(Workout* w);

/**
 * Calculate total volume from comma-separated weights and reps
 * 
 * Volume = sum of (weight × reps) for each set
 * Example: weights="100,100,95,90", reps="8,8,7,6" -> 3675
 * 
 * Returns: total volume in kg-reps, or -1.0 on parse error
 */
double calculate_volume(const char* weights, const char* reps);

/**
 * Extract maximum weight from comma-separated weights string
 * 
 * Example: weights="100,100,95,90" -> 100.0
 * 
 * Returns: max weight, or -1.0 on parse error
 */
double get_max_weight(const char* weights);

/**
 * Free all allocated memory for a workout struct
 */
void free_workout(Workout* w);

#endif /* WORKOUT_H */
