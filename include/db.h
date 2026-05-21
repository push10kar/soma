#ifndef DB_H
#define DB_H

#include <sqlite3.h>

sqlite3 *db_open(void);

void db_close(sqlite3 *db);

void db_init(sqlite3 *db);

#endif