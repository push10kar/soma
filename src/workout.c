#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <sqlite3.h>
#include "workout.h"
#include "db.h"

/* Helper: Duplicate a string */
static char* str_dup(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str) + 1;
    char* dup = (char*)malloc(len);
    if (dup) memcpy(dup, str, len);
    return dup;
}
static bool is_valid_number(const char* str, double* out) {
    if (!str || *str == '\0') return false;
    
    char* end;
    double val = strtod(str, &end);
    
    if (*end != '\0' || val <= 0) return false;
    if (out) *out = val;
    return true;
}

/* Helper: Check if a string is a valid positive integer */
static bool is_valid_integer(const char* str, int* out) {
    if (!str || *str == '\0') return false;
    
    char* end;
    long val = strtol(str, &end, 10);
    
    if (*end != '\0' || val <= 0) return false;
    if (out) *out = (int)val;
    return true;
}

/* Helper: Count commas in a string (i.e., number of items) */
static int count_items(const char* str) {
    if (!str || *str == '\0') return 0;
    
    int count = 1;
    for (const char* p = str; *p; p++) {
        if (*p == ',') count++;
    }
    return count;
}

/* Helper: Parse comma-separated integers into an array */
static int* parse_int_array(const char* str, int* out_len) {
    if (out_len) *out_len = 0;
    if (!str || *str == '\0') return NULL;
    
    int count = count_items(str);
    int* arr = (int*)malloc(count * sizeof(int));
    if (!arr) return NULL;
    
    char* dup = str_dup(str);
    if (!dup) {
        free(arr);
        return NULL;
    }
    
    int idx = 0;
    char* token = strtok(dup, ",");
    while (token && idx < count) {
        /* Trim whitespace */
        while (*token && isspace(*token)) token++;
        char* end = token + strlen(token) - 1;
        while (end >= token && isspace(*end)) *end-- = '\0';
        
        if (!is_valid_integer(token, &arr[idx])) {
            free(arr);
            free(dup);
            return NULL;
        }
        idx++;
        token = strtok(NULL, ",");
    }
    
    free(dup);
    if (out_len) *out_len = idx;
    return arr;
}

/* Helper: Parse comma-separated doubles into an array */
static double* parse_double_array(const char* str, int* out_len) {
    if (out_len) *out_len = 0;
    if (!str || *str == '\0') return NULL;
    
    int count = count_items(str);
    double* arr = (double*)malloc(count * sizeof(double));
    if (!arr) return NULL;
    
    char* dup = str_dup(str);
    if (!dup) {
        free(arr);
        return NULL;
    }
    
    int idx = 0;
    char* token = strtok(dup, ",");
    while (token && idx < count) {
        /* Trim whitespace */
        while (*token && isspace(*token)) token++;
        char* end = token + strlen(token) - 1;
        while (end >= token && isspace(*end)) *end-- = '\0';
        
        if (!is_valid_number(token, &arr[idx])) {
            free(arr);
            free(dup);
            return NULL;
        }
        idx++;
        token = strtok(NULL, ",");
    }
    
    free(dup);
    if (out_len) *out_len = idx;
    return arr;
}

/**
 * Parse command-line arguments for workout logging
 */
Workout* parse_workout_args(int argc, char* argv[]) {
    Workout* w = (Workout*)malloc(sizeof(Workout));
    if (!w) {
        fprintf(stderr, "error: failed to allocate memory for workout\n");
        return NULL;
    }
    
    /* Initialize */
    w->exercise = NULL;
    w->reps = NULL;
    w->weights = NULL;
    w->num_sets = 0;
    w->volume_kg = 0.0;
    w->is_new_pr = false;
    w->error = WORKOUT_OK;
    w->error_msg = NULL;
    
    /* Parse arguments: --exercise NAME --sets N --reps "..." --weight "..." */
    char* exercise = NULL;
    char* reps_str = NULL;
    char* weights_str = NULL;
    int num_sets = 0;
    bool has_sets_flag = false;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--exercise") == 0) {
            if (i + 1 < argc) {
                exercise = argv[++i];
            }
        } else if (strcmp(argv[i], "--sets") == 0) {
            if (i + 1 < argc) {
                if (!is_valid_integer(argv[++i], &num_sets)) {
                    w->error = WORKOUT_ERR_VALIDATION_FAILED;
                    w->error_msg = str_dup("--sets must be a positive integer");
                    return w;
                }
                has_sets_flag = true;
            }
        } else if (strcmp(argv[i], "--reps") == 0) {
            if (i + 1 < argc) {
                reps_str = argv[++i];
            }
        } else if (strcmp(argv[i], "--weight") == 0) {
            if (i + 1 < argc) {
                weights_str = argv[++i];
            }
        }
    }
    
    /* Validate required arguments */
    if (!exercise || *exercise == '\0') {
        w->error = WORKOUT_ERR_VALIDATION_FAILED;
        w->error_msg = str_dup("--exercise is required");
        return w;
    }
    
    if (!has_sets_flag || num_sets <= 0) {
        w->error = WORKOUT_ERR_VALIDATION_FAILED;
        w->error_msg = str_dup("--sets is required and must be positive");
        return w;
    }
    
    if (!reps_str || *reps_str == '\0') {
        w->error = WORKOUT_ERR_VALIDATION_FAILED;
        w->error_msg = str_dup("--reps is required");
        return w;
    }
    
    if (!weights_str || *weights_str == '\0') {
        w->error = WORKOUT_ERR_VALIDATION_FAILED;
        w->error_msg = str_dup("--weight is required");
        return w;
    }
    
    /* Validate counts match */
    int reps_count = count_items(reps_str);
    int weights_count = count_items(weights_str);
    
    if (reps_count != num_sets || weights_count != num_sets) {
        w->error = WORKOUT_ERR_VALIDATION_FAILED;
        w->error_msg = str_dup("--sets count doesn't match reps and weights count");
        return w;
    }
    
    /* Parse reps and weights to validate they are numbers */
    int reps_len = 0, weights_len = 0;
    int* reps_arr = parse_int_array(reps_str, &reps_len);
    double* weights_arr = parse_double_array(weights_str, &weights_len);
    
    if (!reps_arr || !weights_arr || reps_len != num_sets || weights_len != num_sets) {
        w->error = WORKOUT_ERR_VALIDATION_FAILED;
        w->error_msg = str_dup("Failed to parse reps or weights");
        free(reps_arr);
        free(weights_arr);
        return w;
    }
    
    /* Calculate volume */
    double total_volume = 0.0;
    for (int i = 0; i < num_sets; i++) {
        total_volume += weights_arr[i] * reps_arr[i];
    }
    
    /* Populate workout struct */
    w->exercise = str_dup(exercise);
    w->reps = str_dup(reps_str);
    w->weights = str_dup(weights_str);
    w->num_sets = num_sets;
    w->volume_kg = total_volume;
    w->error = WORKOUT_OK;
    
    free(reps_arr);
    free(weights_arr);
    
    return w;
}

/**
 * Calculate total volume from comma-separated weights and reps
 */
double calculate_volume(const char* weights, const char* reps) {
    if (!weights || !reps) return -1.0;
    
    int weights_len = 0, reps_len = 0;
    double* weights_arr = parse_double_array(weights, &weights_len);
    int* reps_arr = parse_int_array(reps, &reps_len);
    
    if (!weights_arr || !reps_arr || weights_len != reps_len) {
        free(weights_arr);
        free(reps_arr);
        return -1.0;
    }
    
    double total = 0.0;
    for (int i = 0; i < weights_len; i++) {
        total += weights_arr[i] * reps_arr[i];
    }
    
    free(weights_arr);
    free(reps_arr);
    
    return total;
}

/**
 * Log a workout to the database and detect personal records
 */
bool log_workout(Workout* w) {
    if (!w) return false;
    
    if (w->error != WORKOUT_OK) {
        fprintf(stderr, "error: workout has validation errors\n");
        return false;
    }
    
    fprintf(stderr, "DEBUG: Opening database\n");
    /* Open database */
    sqlite3* db = db_open();
    if (!db) {
        fprintf(stderr, "DEBUG: db_open returned NULL\n");
        return false;
    }
    
    fprintf(stderr, "DEBUG: Parsing weights array\n");
    /* Find max weight in this workout to compare against PR */
    int weights_len = 0;
    double* weights_arr = parse_double_array(w->weights, &weights_len);
    if (!weights_arr) {
        fprintf(stderr, "DEBUG: parse_double_array returned NULL\n");
        db_close(db);
        return false;
    }
    
    fprintf(stderr, "DEBUG: Finding max weight, weights_len=%d\n", weights_len);
    double max_weight = weights_arr[0];
    for (int i = 1; i < weights_len; i++) {
        if (weights_arr[i] > max_weight) {
            max_weight = weights_arr[i];
        }
    }
    free(weights_arr);
    
    fprintf(stderr, "DEBUG: Querying for existing PR for exercise: %s\n", w->exercise);
    /* Query personal_records to find current PR for this exercise */
    double current_pr_weight = 0.0;
    bool has_existing_pr = false;
    
    const char* query_pr = "SELECT weight_kg FROM personal_records WHERE exercise = ?";
    sqlite3_stmt* stmt_pr;
    
    if (sqlite3_prepare_v2(db, query_pr, -1, &stmt_pr, NULL) == SQLITE_OK) {
        sqlite3_bind_text(stmt_pr, 1, w->exercise, -1, SQLITE_STATIC);
        if (sqlite3_step(stmt_pr) == SQLITE_ROW) {
            current_pr_weight = sqlite3_column_double(stmt_pr, 0);
            has_existing_pr = true;
        }
        sqlite3_finalize(stmt_pr);
    }
    
    fprintf(stderr, "DEBUG: Checking PR: max_weight=%.2f, current_pr=%.2f, has_existing=%d\n", 
            max_weight, current_pr_weight, has_existing_pr);
    
    /* Check if this is a new PR */
    w->is_new_pr = false;
    if (!has_existing_pr || max_weight > current_pr_weight) {
        w->is_new_pr = true;
        
        fprintf(stderr, "DEBUG: New PR detected, updating personal_records\n");
        
        /* Find the rep count for the max weight set */
        int* reps_arr = parse_int_array(w->reps, NULL);
        double* weights_arr_temp = parse_double_array(w->weights, NULL);
        int reps_for_max = 1;
        
        for (int i = 0; i < w->num_sets; i++) {
            if (weights_arr_temp && reps_arr && weights_arr_temp[i] == max_weight) {
                reps_for_max = reps_arr[i];
                break;
            }
        }
        
        free(weights_arr_temp);
        free(reps_arr);
        
        double estimated_1rm = max_weight * (1.0 + reps_for_max / 30.0);

        /* Insert or update personal_records */
        const char* insert_pr = 
            "INSERT OR REPLACE INTO personal_records "
            "(exercise, weight_kg, reps, estimated_1rm, is_new_pr, achieved_at) "
            "VALUES (?, ?, ?, ?, 1, datetime('now','localtime'))";
        
        sqlite3_stmt* stmt_insert;
        fprintf(stderr, "DEBUG: Preparing PR insert statement\n");
        if (sqlite3_prepare_v2(db, insert_pr, -1, &stmt_insert, NULL) == SQLITE_OK) {
            fprintf(stderr, "DEBUG: Binding PR values\n");
            sqlite3_bind_text(stmt_insert, 1, w->exercise, -1, SQLITE_STATIC);
            sqlite3_bind_double(stmt_insert, 2, max_weight);
            sqlite3_bind_int(stmt_insert, 3, reps_for_max);
            sqlite3_bind_double(stmt_insert, 4, estimated_1rm);
            
            fprintf(stderr, "DEBUG: Stepping PR insert\n");
            if (sqlite3_step(stmt_insert) != SQLITE_DONE) {
                fprintf(stderr, "error: failed to insert personal record: %s\n", 
                        sqlite3_errmsg(db));
                sqlite3_finalize(stmt_insert);
                db_close(db);
                return false;
            }
            sqlite3_finalize(stmt_insert);
        } else {
            fprintf(stderr, "error: failed to prepare PR insert statement\n");
            db_close(db);
            return false;
        }
    }
    
    fprintf(stderr, "DEBUG: Preparing workout insert statement\n");
    /* Insert into workouts table */
    const char* insert_workout =
        "INSERT INTO workouts (exercise, weight_kg, reps, volume_kg) "
        "VALUES (?, ?, ?, ?)";
    
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, insert_workout, -1, &stmt, NULL) != SQLITE_OK) {
        fprintf(stderr, "error: failed to prepare workout insert: %s\n", 
                sqlite3_errmsg(db));
        db_close(db);
        return false;
    }
    
    fprintf(stderr, "DEBUG: Binding workout values\n");
    sqlite3_bind_text(stmt, 1, w->exercise, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, w->weights, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, w->reps, -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 4, w->volume_kg);
    
    fprintf(stderr, "DEBUG: Stepping workout insert\n");
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "error: failed to insert workout: %s\n", 
                sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        db_close(db);
        return false;
    }
    
    fprintf(stderr, "DEBUG: Finalizing and closing\n");
    sqlite3_finalize(stmt);
    db_close(db);
    
    fprintf(stderr, "DEBUG: Success\n");
    return true;
}

/**
 * Extract maximum weight from comma-separated weights string
 */
double get_max_weight(const char* weights) {
    if (!weights) return -1.0;
    
    int weights_len = 0;
    double* weights_arr = parse_double_array(weights, &weights_len);
    
    if (!weights_arr || weights_len == 0) {
        free(weights_arr);
        return -1.0;
    }
    
    double max_weight = weights_arr[0];
    for (int i = 1; i < weights_len; i++) {
        if (weights_arr[i] > max_weight) {
            max_weight = weights_arr[i];
        }
    }
    
    free(weights_arr);
    return max_weight;
}

/**
 * Free all allocated memory for a workout struct
 */
void free_workout(Workout* w) {
    if (!w) return;
    
    if (w->exercise) free(w->exercise);
    if (w->reps) free(w->reps);
    if (w->weights) free(w->weights);
    if (w->error_msg) free(w->error_msg);
    
    free(w);
}

static const char* resolve_exercise_name(const char* name) {
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

Workout* parse_workout_shorthand(int argc, char* argv[]) {
    Workout* w = (Workout*)malloc(sizeof(Workout));
    if (!w) {
        return NULL;
    }
    w->exercise = NULL;
    w->reps = NULL;
    w->weights = NULL;
    w->num_sets = 0;
    w->volume_kg = 0.0;
    w->is_new_pr = false;
    w->error = WORKOUT_OK;
    w->error_msg = NULL;

    if (argc < 2) {
        w->error = WORKOUT_ERR_VALIDATION_FAILED;
        w->error_msg = str_dup("Invalid shorthand logging arguments. Usage: soma log <exercise> <weight>x<reps>x<sets>");
        return w;
    }

    const char* expr = argv[argc - 1];
    
    size_t ex_size = 0;
    for (int i = 0; i < argc - 1; i++) {
        ex_size += strlen(argv[i]) + 1;
    }
    
    char* exercise_raw = (char*)malloc(ex_size);
    if (!exercise_raw) {
        w->error = WORKOUT_ERR_MEMORY;
        w->error_msg = str_dup("Memory allocation failed");
        return w;
    }
    exercise_raw[0] = '\0';
    for (int i = 0; i < argc - 1; i++) {
        strcat(exercise_raw, argv[i]);
        if (i < argc - 2) {
            strcat(exercise_raw, " ");
        }
    }

    const char* resolved = resolve_exercise_name(exercise_raw);
    w->exercise = str_dup(resolved);
    free(exercise_raw);

    double weight = 0.0;
    int reps = 0;
    int sets = 0;
    
    char expr_lower[64];
    strncpy(expr_lower, expr, sizeof(expr_lower));
    expr_lower[sizeof(expr_lower) - 1] = '\0';
    for (int i = 0; expr_lower[i]; i++) {
        if (expr_lower[i] == 'X') expr_lower[i] = 'x';
    }

    if (sscanf(expr_lower, "%lfx%dx%d", &weight, &reps, &sets) != 3) {
        w->error = WORKOUT_ERR_PARSE_FAILED;
        w->error_msg = str_dup("Invalid shorthand expression format. Expected <weight>x<reps>x<sets> (e.g. 80x5x3)");
        return w;
    }

    if (weight <= 0.0 || reps <= 0 || sets <= 0) {
        w->error = WORKOUT_ERR_VALIDATION_FAILED;
        w->error_msg = str_dup("Weight, reps, and sets must be positive numbers");
        return w;
    }

    w->num_sets = sets;

    size_t reps_len = (16 + 1) * sets;
    size_t weights_len = (32 + 1) * sets;
    w->reps = (char*)malloc(reps_len);
    w->weights = (char*)malloc(weights_len);
    if (!w->reps || !w->weights) {
        if (w->reps) free(w->reps);
        if (w->weights) free(w->weights);
        w->error = WORKOUT_ERR_MEMORY;
        w->error_msg = str_dup("Memory allocation failed");
        return w;
    }

    w->reps[0] = '\0';
    w->weights[0] = '\0';

    char rep_item[16];
    char weight_item[32];
    snprintf(rep_item, sizeof(rep_item), "%d", reps);
    if (weight == (int)weight) {
        snprintf(weight_item, sizeof(weight_item), "%.0f", weight);
    } else {
        snprintf(weight_item, sizeof(weight_item), "%.1f", weight);
    }

    for (int i = 0; i < sets; i++) {
        strcat(w->reps, rep_item);
        strcat(w->weights, weight_item);
        if (i < sets - 1) {
            strcat(w->reps, ",");
            strcat(w->weights, ",");
        }
    }

    w->volume_kg = weight * reps * sets;

    return w;
}
