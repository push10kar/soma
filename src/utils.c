#include <stdio.h>
#include <string.h>
#include "utils.h"

void print_logo(void) {
    printf("\n");
    printf(ACCENT BOLD "  soma" RESET
           DIM "  ·  terminal fitness tracker" RESET "\n");
    printf("\n");
}

void print_separator(void) {
    printf(DIM "  ──────────────────────────────" RESET "\n");
}

void print_thin_sep(void) {
    printf("\n");
}

void print_section(const char *title) {
    printf("\n");
    printf(ACCENT "  ▸ " RESET);
    printf(MUTED "%s" RESET "\n", title);
    printf("\n");
}

void print_row(const char *label, const char *value, const char *delta) {
    printf(DIM "  %-20s" RESET, label);
    printf(ACCENT BOLD "%-14s" RESET, value);
    if (delta && delta[0] != '\0') {
        if (delta[0] == '+')
            printf(POSITIVE "%s" RESET, delta);
        else if (delta[0] == '-')
            printf(POSITIVE "%s" RESET, delta);  /* losing cm/fat = good */
        else
            printf(WARNING "%s" RESET, delta);
    }
    printf("\n");
}

void print_row_neg(const char *label, const char *value, const char *delta) {
    /* use this when - means bad (weight going up on a cut, etc) */
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

void print_verdict(const char *msg) {
    printf("\n");
    print_separator();
    printf("\n");
    printf(ACCENT "  ❯ " RESET BOLD WHITE "%s" RESET "\n", msg);
    printf("\n");
}

/* sparkline with spaces between chars to fix double-width font issue */
void print_sparkline(double *values, int count) {
    const char *blocks[] = {
        "\xe2\x96\x81",
        "\xe2\x96\x82",
        "\xe2\x96\x83",
        "\xe2\x96\x84",
        "\xe2\x96\x85",
        "\xe2\x96\x86",
        "\xe2\x96\x87",
        "\xe2\x96\x88"
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
        printf("%s ", blocks[idx]);   /* space after each char — fixes width */
    }
    printf(RESET);
}

void print_bar(const char *label, double value,
               double target, const char *unit) {
    int total  = 16;
    int filled = (int)((value / target) * total);
    if (filled > total) filled = total;
    int empty  = total - filled;

    double ratio = value / target;
    const char *color;
    if (ratio >= 0.95)      color = ACCENT;
    else if (ratio >= 0.80) color = WARNING;
    else                    color = NEGATIVE;

    printf(DIM "  %-20s" RESET, label);
    printf(color);
    for (int i = 0; i < filled; i++) printf("▪");
    printf(RESET DIM);
    for (int i = 0; i < empty;  i++) printf("·");

    /* percentage */
    int pct = (int)(ratio * 100);
    if (pct > 100) pct = 100;

    if (ratio >= 0.95)
        printf(ACCENT "  %3d%%" RESET, pct);
    else if (ratio >= 0.80)
        printf(WARNING "  %3d%%" RESET, pct);
    else
        printf(NEGATIVE "  %3d%%" RESET, pct);

    printf(DIM "  %.0f/%.0f%s" RESET "\n", value, target, unit);
}

void print_log_row(const char *label, const char *value, const char *note) {
    printf(DIM "  %-20s" RESET, label);
    printf(WHITE "%-16s" RESET, value);
    if (note && note[0] != '\0')
        printf(POSITIVE "%s" RESET, note);
    printf("\n");
}

void print_pr_badge(void) {
    printf("\n");
    printf(ACCENT
           "  ╔═══════════════════════════╗\n"
           "  ║   ↑  new personal record  ║\n"
           "  ╚═══════════════════════════╝"
           RESET "\n\n");
}

void print_hist_row(const char *date, const char *value,
                    const char *volume, int is_today) {
    if (is_today) {
        printf(DIM "  %-12s" RESET, date);
        printf(ACCENT BOLD "%-20s" RESET, value);
        printf(DIM "%-10s" RESET, volume);
        printf(ACCENT "← today" RESET "\n");
    } else {
        printf(DIM "  %-12s" RESET, date);
        printf(WHITE "%-20s" RESET, value);
        printf(DIM "%s" RESET "\n", volume);
    }
}