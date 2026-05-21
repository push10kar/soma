#include <stdio.h>
#include "db.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    sqlite3 *db = db_open();
    printf("soma — database ready\n");
    db_close(db);
    return 0;
}