#include <stdio.h>
#include <string.h>
#include <time.h>
#include "utils.h"

/* ── date helper ─────────────────────────────────────── */
static void print_date(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char day[16], mon[16];

    /* day name */
    const char *days[] = {
        "sunday","monday","tuesday","wednesday",
        "thursday","friday","saturday"
    };
    const char *months[] = {
        "jan","feb","mar","apr","may","jun",
        "jul","aug","sep","oct","nov","dec"
    };

    snprintf(day, sizeof(day), "%s", days[tm->tm_wday]);
    snprintf(mon, sizeof(mon), "%s", months[tm->tm_mon]);

    printf(DIM "%s  ·  %s %d" RESET,
           day, mon, tm->tm_mday);
}

/* ── logo ────────────────────────────────────────────── */
void print_logo(void) {
    printf("\n");
    printf(ACCENT BOLD "  soma" RESET);
    printf(DIM "  ·  " RESET);
    print_date();
    printf("\n\n");
}

/* ── separators ──────────────────────────────────────── */
void print_separator(void) {
    printf(DIM "  ·····································"
               "·······" RESET "\n");
}

void print_thin_sep(void) {
    printf("\n");
}

/* ── section title ───────────────────────────────────── */
void print_section(const char *title) {
    printf("\n");
    printf(ACCENT "  ▸ " RESET);
    printf(DIM "%s" RESET "\n\n", title);
}

/* ── standard row ────────────────────────────────────── */
void print_row(const char *label, const char *value,
               const char *delta) {
    printf(DIM "  %-20s" RESET, label);
    printf(ACCENT BOLD "%-14s" RESET, value);
    if (delta && delta[0] != '\0') {
        if (delta[0] == '+')
            printf(POSITIVE "%s" RESET, delta);
        else if (delta[0] == '-')
            printf(POSITIVE "%s" RESET, delta);
        else
            printf(WARNING "%s" RESET, delta);
    }
    printf("\n");
}

/* ── row where + is bad (gaining weight on a cut etc) ── */
void print_row_neg(const char *label, const char *value,
                   const char *delta) {
    printf(DIM "  %-20s" RESET, label);
    printf(ACCENT BOLD "%-14s" RESET, value);
    if (delta && delta[0] != '\0') {
        if (delta[0] == '+')
            printf(NEGATIVE "%s" RESET, delta);
        else if (delta[0] == '-')
            printf(POSITIVE "%s" RESET, delta);
        else
            printf(WARNING "%s" RESET, delta);
    }
    printf("\n");
}

/* ── verdict ─────────────────────────────────────────── */
void print_verdict(const char *msg) {
    printf("\n");
    print_separator();
    printf("\n");
    printf(ACCENT "  ❯ " RESET BOLD WHITE "%s" RESET "\n\n", msg);
}

/* ── sparkline ───────────────────────────────────────── */
void print_sparkline(double *values, int count) {
    const char *blocks[] = {
        "\xe2\x96\x81", "\xe2\x96\x82",
        "\xe2\x96\x83", "\xe2\x96\x84",
        "\xe2\x96\x85", "\xe2\x96\x86",
        "\xe2\x96\x87", "\xe2\x96\x88"
    };

    if (count <= 0) return;

    double min = values[0], max = values[0];
    for (int i = 1; i < count; i++) {
        if (values[i] < min) min = values[i];
        if (values[i] > max) max = values[i];
    }

    double range = max - min;
    printf(ACCENT);
    for (int i = 0; i < count; i++) {
        int idx;
        if (range == 0.0)
            idx = 3;
        else
            idx = (int)((values[i] - min) / range * 7.0 + 0.5);
        if (idx < 0) idx = 0;
        if (idx > 7) idx = 7;
        printf("%s ", blocks[idx]);
    }
    printf(RESET);
}

/* ── nutrition bar with deficit ─────────────────────── */
void print_bar(const char *label, double value,
               double target, const char *unit) {
    int total  = 16;
    int filled = (int)((value / target) * total);
    if (filled > total) filled = total;
    int empty  = total - filled;

    double ratio   = value / target;
    double deficit = target - value;

    const char *color;
    if (ratio >= 0.95)      color = ACCENT;
    else if (ratio >= 0.80) color = WARNING;
    else                    color = NEGATIVE;

    printf(DIM "  %-20s" RESET, label);
    printf(color);
    for (int i = 0; i < filled; i++) printf("▪");
    printf(RESET DIM);
    for (int i = 0; i < empty;  i++) printf("·");
    printf(RESET);

    printf(DIM "  " RESET);
    printf("%s", color);
    printf("%.0f/%.0f%s", value, target, unit);
    printf("%s", RESET);

    /* show deficit if not at target */
    if (ratio < 0.99) {
        printf(DIM "  -%.0f%s" RESET, deficit, unit);
    } else {
        printf(POSITIVE "  ✓" RESET);
    }

    printf("\n\n");
}

/* ── log row (after soma bench 80x5) ────────────────── */
void print_log_row(const char *label, const char *value,
                   const char *note) {
    printf(DIM "  %-20s" RESET, label);
    printf(WHITE "%-16s" RESET, value);
    if (note && note[0] != '\0')
        printf(POSITIVE "%s" RESET, note);
    printf("\n");
}

/* ── PR notification ─────────────────────────────────── */
void print_pr_badge(const char *exercise, double orm) {
    printf("\n");
    printf(ACCENT "  ↑ personal record" RESET);
    printf(DIM "  ·  %s  ·  est. 1rm %.1fkg" RESET "\n\n",
           exercise, orm);
}

/* ── history row (soma last bench) ──────────────────── */
void print_hist_row(const char *date, const char *value,
                    const char *volume, int is_today) {
    if (is_today) {
        printf(DIM  "  %-12s" RESET, date);
        printf(ACCENT BOLD "%-20s" RESET, value);
        printf(DIM  "%-10s" RESET, volume);
        printf(ACCENT "← today" RESET "\n");
    } else {
        printf(DIM  "  %-12s" RESET, date);
        printf(WHITE "%-20s" RESET, value);
        printf(DIM  "%s" RESET "\n", volume);
    }
}

/* ── volume sparkline row ────────────────────────────── */
void print_volume_trend(double *volumes, int count,
                        double pct_change) {
    printf(DIM "  %-20s" RESET, "volume trend");
    print_sparkline(volumes, count);
    if (pct_change >= 0)
        printf(POSITIVE "  +%.1f%% this month" RESET "\n", pct_change);
    else
        printf(NEGATIVE "  %.1f%% this month"  RESET "\n", pct_change);
    printf("\n");
}

/* ── today command output ────────────────────────────── */
void print_today_header(int days_since, const char *last_day,
                        double sleep_hrs, int sleep_logged,
                        int weight_logged) {
    printf("\n");

    /* last trained */
    if (days_since == 0)
        printf(DIM "  last trained    " RESET
               ACCENT "today\n" RESET);
    else if (days_since == 1)
        printf(DIM "  last trained    " RESET
               ACCENT "yesterday" RESET
               DIM "  (%s)\n" RESET, last_day);
    else
        printf(DIM "  last trained    " RESET
               WARNING "%d days ago" RESET
               DIM "  (%s)\n" RESET, days_since, last_day);

    /* bodyweight */
    if (weight_logged)
        printf(DIM "  bodyweight      " RESET
               ACCENT "logged  ✓\n" RESET);
    else
        printf(DIM "  bodyweight      " RESET
               DIM "not logged today\n" RESET);

    /* sleep */
    if (sleep_logged) {
        if (sleep_hrs >= 7.0)
            printf(DIM "  sleep           " RESET
                   POSITIVE "%.1fh  ✓\n" RESET, sleep_hrs);
        else
            printf(DIM "  sleep           " RESET
                   WARNING "%.1fh  — under target\n" RESET, sleep_hrs);
    } else {
        printf(DIM "  sleep           " RESET
               DIM "not logged today\n" RESET);
    }

    printf("\n");
}