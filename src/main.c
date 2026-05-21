#include <stdio.h>
#include "db.h"
#include "utils.h"

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    sqlite3 *db = db_open();

    print_logo();
    print_separator();

    /* ── training ── */
    print_section("training");

    double bench[] = {72.5, 75.0, 75.0, 77.5, 77.5, 80.0};
    double squat[] = {100.0, 105.0, 107.5, 110.0, 115.0, 120.0};
    double ohp[]   = {45.0, 47.5, 47.5, 50.0, 50.0, 52.0};

    printf(DIM "  %-20s%-14s%-16s%s\n\n" RESET,
           "exercise", "last set", "trend", "est. 1rm");

    printf(DIM "  %-20s" RESET, "bench press");
    printf(ACCENT BOLD "%-14s" RESET, "80kg × 5");
    print_sparkline(bench, 6);
    printf(POSITIVE "  93.3kg\n\n" RESET);   /* \n\n */

    printf(DIM "  %-20s" RESET, "squat");
    printf(ACCENT BOLD "%-14s" RESET, "120kg × 5");
    print_sparkline(squat, 6);
    printf(POSITIVE "  139.9kg\n\n" RESET);  /* \n\n */

    printf(DIM "  %-20s" RESET, "overhead press");
    printf(ACCENT BOLD "%-14s" RESET, "52kg × 5");
    print_sparkline(ohp, 6);
    printf(POSITIVE "  60.4kg\n" RESET);     /* last row single \n */

    /* ── physique ── */
    print_section("physique");

    double wt[] = {81.0, 80.5, 80.2, 79.8, 79.5, 79.2};
    printf(DIM "  %-20s" RESET, "bodyweight");
    printf(ACCENT BOLD "%-14s" RESET, "79.2kg");
    print_sparkline(wt, 6);
    printf(POSITIVE "  -0.4kg/wk\n\n" RESET);  /* \n\n */

    print_row("waist", "83cm", "-1cm");

    /* ── recovery ── */
    print_section("recovery");

    double sl[] = {6.5, 7.0, 7.5, 6.0, 7.5, 8.0};
   printf(DIM "  %-20s" RESET, "sleep 7d avg");
    printf(ACCENT BOLD "%-14s" RESET, "7.4h");
    print_sparkline(sl, 6);
    printf(DIM "  target 7.5h\n\n" RESET);     /* \n\n */

    printf(DIM "  %-20s" RESET, "readiness");
    printf(ACCENT BOLD "%-14s" RESET, "71 / 100");
    printf(POSITIVE "good to train\n" RESET);

    /* ── nutrition ── */
    print_section("nutrition");
    print_bar("protein",  178.0, 200.0, "g");
    print_bar("calories", 2650.0, 2800.0, "kcal");
    print_bar("carbs",    280.0, 300.0, "g");
    print_bar("fat",       55.0,  70.0, "g");

    /* ── verdict ── */
    print_verdict("progressing well — bench stalling 2 weeks, "
                  "attempt 82.5kg next session.");

    /* ── log output demo ── */
    printf(DIM "\n  $ soma bench 80x5\n\n" RESET);
    print_separator();
    printf("\n");
    printf(ACCENT BOLD "  bench press\n\n" RESET);
    print_log_row("logged",      "80kg × 5",    "");
    print_log_row("last",        "77.5kg × 5",  "+2.5kg");
    print_log_row("est. 1rm",    "93.3kg",      "+3.1kg");
    print_log_row("volume",      "400kg",       "");
    print_log_row("session vol", "1,160kg",     "");
    print_pr_badge();

    /* ── history demo ── */
    printf(DIM "  $ soma last bench\n\n" RESET);
    print_separator();
    printf("\n");
    printf(ACCENT BOLD "  bench press" RESET
           DIM " — last 5 sessions\n\n" RESET);

    print_hist_row("may 14", "80kg × 5",   "400kg",  1);
    print_hist_row("may 10", "77.5kg × 5", "387kg",  0);
    print_hist_row("may 06", "77.5kg × 5", "387kg",  0);
    print_hist_row("may 01", "75kg × 5",   "375kg",  0);
    print_hist_row("apr 27", "75kg × 5",   "375kg",  0);

    printf("\n");
    printf(DIM "  %-12s" RESET ACCENT "+1.6kg / week\n" RESET, "trend");
    printf(DIM "  %-12s" RESET ACCENT BOLD "82.5kg × 5" RESET
           DIM "  next session\n" RESET, "target");
    printf("\n");

    db_close(db);
    return 0;
}