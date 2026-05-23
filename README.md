# soma

![C](https://img.shields.io/badge/language-C-555?style=flat-square&logo=c&logoColor=white&labelColor=111)
![SQLite](https://img.shields.io/badge/database-SQLite-555?style=flat-square&logo=sqlite&logoColor=white&labelColor=111)
![Linux](https://img.shields.io/badge/platform-Linux-555?style=flat-square&logo=linux&logoColor=white&labelColor=111)
![License](https://img.shields.io/badge/license-MIT-555?style=flat-square&labelColor=111)
![Status](https://img.shields.io/badge/status-actively%20used-b8ff00?style=flat-square&labelColor=111&color=b8ff00)

**a terminal fitness tracker that doesn't try to be your friend.**

built in C because I wanted something fast, honest, and fully mine. no accounts, no cloud, no dopamine tricks. just a tool that tells you whether you're actually making progress.

```
soma  ·  thursday  ·  may 21

  ▸ training

  bench press     82.5kg × 5    ▁ ▂ ▃ ▅ ▆ █    95.9kg

  squat           122.5kg × 5   ▁ ▁ ▂ ▄ ▅ █   142.4kg
  
  overhead press  55kg × 5      ▁ ▂ ▂ ▃ ▄ █    63.9kg

  ❯ progressing well — bench up 12.5kg in 10 weeks.
```

---

## why a terminal app

most fitness apps optimize for engagement, not results. they want you opening the app, not closing it. soma does the opposite — you log a set in 3 seconds and close it. the data is yours, stored locally in a plain SQLite file that will be readable in 30 years.

> make the minimum system that reliably changes behavior. that's the real product.

---

## install

**dependencies**

```bash
# fedora
sudo dnf install gcc make sqlite-devel sqlite

# ubuntu / debian
sudo apt install gcc make libsqlite3-dev sqlite3
```

**build**

```bash
git clone https://github.com/push10kar/soma
cd soma
make
sudo make install   # copies binary to /usr/local/bin
```

**verify**

```bash
soma
# soma  ·  thursday  ·  may 21
```

your database is created automatically at `~/.soma/soma.db` on first run. nothing else to configure.

tab completion is set up automatically by `make install`.
restart your shell or run `source ~/.bashrc` to activate it.

---

## commands

### daily use

```bash
soma                          # today's briefing — run this every morning
soma status                   # full dashboard — all metrics, all lifts
```

**`soma`** shows a compact view: what split day it is, last trained, sleep logged, bodyweight logged, and one directive for today.

**`soma status`** is the full picture — training trends, physique metrics, recovery, nutrition, and a verdict. run this on Sundays.

---

### logging

```bash
soma log bench 80x5x3         # bench press — 80kg, 5 reps, 3 sets
soma log squat 100x5          # squat — 100kg, 5 reps, 1 set
soma bodyweight log 79.2      # log bodyweight in kg
soma bodyweight log 79.2 83   # bodyweight + waist measurement
soma sleep log 7.5 4          # sleep hours + quality (1-5)
soma nutrition log 2800 200 300 70
#                  ^    ^   ^   ^
#               kcal protein carbs fat (grams)
```

after logging a workout set soma immediately shows:

```
  bench press

  logged        80kg × 5
  last          77.5kg × 5    +2.5kg
  est. 1rm      93.3kg        +3.1kg
  volume        400kg

  ↑ personal record  ·  bench press  ·  93.3kg est. 1rm
```

---

### analytics

```bash
soma history bench            # last 5 sessions for an exercise
soma suggest bench            # what weight to attempt next session
soma physique                 # bodyweight and measurement trends
soma recovery                 # sleep trends and muscle readiness
soma nutrition                # today's macros vs targets
```

**`soma suggest`** is where it gets interesting. it pulls your last 4 sessions, calculates the trend, and gives you one specific target — not a range, not "it depends." if you're stalling it tells you that too.

```
  bench press — progression

  may 14    80kg × 5     ← today
  may 10    77.5kg × 5
  may 06    77.5kg × 5
  may 01    75kg × 5
  apr 27    75kg × 5

  trend     +1.6kg / week
  target    82.5kg × 5    next session
```

---

### split tracking

```bash
soma split setup              # first-time wizard — sets up your training split
soma split show               # view your split with last/next dates
```

the setup wizard asks you a series of questions about your split — how many days, rotation vs fixed weekdays, which exercises belong to each day. takes about 2 minutes. stored permanently, never asked again.

once configured, `soma today` knows what day it is in your split and warns you if you're training the same muscle group too soon.

```
  soma split show

  your split  ·  5 day rotate

  day 1  push    last may 18  ·  next today
  day 2  pull    last may 19  ·  next tomorrow
  day 3  legs    last may 20  ·  next may 22
  day 4  upper   last may 15  ·  next may 23
  day 5  lower   last may 16  ·  next may 24
```

---

### progress and records

```bash
soma prs                      # personal records board
soma streak                   # consistency streaks across all habits
soma weekly                   # weekly review — run every sunday
```

**`soma prs`** shows every exercise and your all-time best:

```
  exercise          weight    reps   est. 1rm   date
  bench press       82.5kg    5      95.9kg     may 14
  squat             122.5kg   5      142.4kg    may 12
  overhead press    55kg      5      63.9kg     may 10
```

**`soma weekly`** is the sunday ritual. one command gives you workout adherence, macro averages, sleep average, weight change, and PRs for the week:

```
  week  may 12 – 18

  workouts      4 / 4     ████████████████  100%
  avg protein   178/200g  ██████████████··   89%  -22g/day
  avg sleep     7.4/7.5h  ███████████████·   98%
  bodyweight    79.4 → 79.0kg  -0.4kg

  prs this week
  bench press   82.5kg × 5

  ❯ solid week. protein slightly under — easy fix.
```

**`soma streak`** tracks consecutive days you logged each habit:

```
  workout       18 days
  sleep log     12 days
  nutrition      7 days
  bodyweight    21 days
```

---

## how 1rm estimation works

soma never asks you to attempt a true max. instead it uses the Epley formula on every set you log:

```
1RM = weight × (1 + reps / 30)
```

so `80kg × 5` gives an estimated 1RM of 93.3kg. tracked over time this is more useful than an actual max — you can see your strength trending upward even when the weight on the bar doesn't change.

---

## data

everything is stored in `~/.soma/soma.db` — a plain SQLite file. you own it completely.

```bash
# export everything to csv
sqlite3 ~/.soma/soma.db -csv "SELECT * FROM workouts;" > workouts.csv

# open in any sqlite browser
# query it however you want
# copy it, back it up, move it
```

no account. no sync. no server. if soma stops working tomorrow your data is still there and readable by any SQLite client.

---

## project structure

```
soma/
├── src/
│   ├── main.c          # arg parsing and command routing
│   ├── db.c            # database connection and schema
│   ├── workout.c       # logging, PR detection, history
│   ├── bodyweight.c    # weight and measurement tracking
│   ├── status.c        # dashboard and verdict logic
│   ├── suggest.c       # progression recommendations
│   ├── split.c         # training split management
│   └── utils.c         # colors, print helpers, sparklines
├── include/
│   └── *.h
├── Makefile
└── README.md
```

---

## philosophy

soma is built around five questions:

- am I progressing?
- am I recovering?
- what should I do next?
- what is failing?
- am I on track for my goal?

every command answers one of these. nothing in soma exists for any other reason.

---

## built by

a student who got tired of fitness apps that optimize for engagement instead of results. built in C as a side project that turned into a daily-use tool.

this is a personal project — it works for my training, my split, my goals. if you want to use it, you'll probably need to adapt it. the code is simple enough that you can.

---

*soma — greek for body.*