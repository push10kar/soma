#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "db.h"

sqlite3 *db_open(void) {
    char dir[512];
    char path[512];

    const char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "error: HOME not set\n");
        exit(1);
    }

    snprintf(dir,  sizeof(dir),  "%s/.soma", home);
    snprintf(path, sizeof(path), "%s/.soma/soma.db", home);

    /* create ~/.soma/ if it doesn't exist */
    mkdir(dir, 0755);

    sqlite3 *db;
    if (sqlite3_open(path, &db) != SQLITE_OK) {
        fprintf(stderr, "error: cannot open database: %s\n",
                sqlite3_errmsg(db));
        exit(1);
    }

    db_init(db);
    return db;
}

void db_close(sqlite3 *db) {
    sqlite3_close(db);
}

void db_init(sqlite3 *db) {
    const char *sql =

        "CREATE TABLE IF NOT EXISTS workouts ("
        "  id          INTEGER PRIMARY KEY,"
        "  exercise    TEXT    NOT NULL,"
        "  weight_kg   REAL    NOT NULL,"
        "  reps        INTEGER NOT NULL,"
        "  volume_kg   REAL    NOT NULL,"
        "  logged_at   TEXT    DEFAULT (datetime('now','localtime'))"
        ");"

        "CREATE TABLE IF NOT EXISTS personal_records ("
        "  exercise      TEXT PRIMARY KEY,"
        "  weight_kg     REAL    NOT NULL,"
        "  reps          INTEGER NOT NULL,"
        "  estimated_1rm REAL    NOT NULL,"
        "  achieved_at   TEXT    NOT NULL"
        ");"

        "CREATE TABLE IF NOT EXISTS bodyweight ("
        "  id          INTEGER PRIMARY KEY,"
        "  weight_kg   REAL NOT NULL,"
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
}