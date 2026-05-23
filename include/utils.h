#ifndef UTILS_H
#define UTILS_H

#define ACCENT   "\033[38;2;184;255;0m"
#define DIM      "\033[38;2;80;80;80m"
#define MUTED    "\033[38;2;120;120;120m"
#define WHITE    "\033[38;2;224;224;224m"
#define POSITIVE "\033[38;2;74;222;128m"
#define WARNING  "\033[38;2;250;204;21m"
#define NEGATIVE "\033[38;2;248;113;113m"
#define BOLD     "\033[1m"
#define RESET    "\033[0m"

#define HM_LEVEL_0 "\033[38;2;22;28;12m"
#define HM_LEVEL_1 "\033[38;2;72;96;20m"
#define HM_LEVEL_2 "\033[38;2;110;150;18m"
#define HM_LEVEL_3 "\033[38;2;148;210;10m"
#define HM_LEVEL_4 "\033[38;2;184;255;0m"

void print_logo(void);
void print_separator(void);
void print_thin_sep(void);
void print_section(const char *title);
void print_row(const char *label, const char *value,
               const char *delta);
void print_row_neg(const char *label, const char *value,
                   const char *delta);
void print_verdict(const char *msg);
void print_sparkline(double *values, int count);
void print_bar(const char *label, double value,
               double target, const char *unit);
void print_log_row(const char *label, const char *value,
                   const char *note);
void print_pr_badge(const char *exercise, double orm);
void print_hist_row(const char *date, const char *value,
                    const char *volume, int is_today);
void print_volume_trend(double *volumes, int count,
                        double pct_change);
void print_today_header(int days_since, const char *last_day,
                        double sleep_hrs, int sleep_logged,
                        int weight_logged);

#endif