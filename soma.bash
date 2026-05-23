_soma_completion() {
    local cur prev pprev
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    pprev="${COMP_WORDS[COMP_CWORD-2]}"

    # ── level 1 — soma [TAB] ────────────────────────────
    local commands="today status weekly streak streaks prs pr
                    physique recovery nutrition suggest history
                    last heatmap activity help split log
                    workout bodyweight sleep"

    if [ "$COMP_CWORD" -eq 1 ]; then
        COMPREPLY=($(compgen -W "$commands" -- "$cur"))
        return 0
    fi

    # ── level 2 — soma <cmd> [TAB] ──────────────────────
    local exercises
    exercises=$(sqlite3 ~/.soma/soma.db \
        "SELECT DISTINCT exercise FROM workouts
         ORDER BY exercise;" 2>/dev/null)

    case "$prev" in

        # commands that take an exercise name
        suggest|history|last)
            COMPREPLY=($(compgen -W "$exercises" -- "$cur"))
            return 0
            ;;

        # log → exercise names (shorthand)
        log)
            COMPREPLY=($(compgen -W "$exercises" -- "$cur"))
            return 0
            ;;

        # split → subcommands
        split)
            COMPREPLY=($(compgen -W "setup show" -- "$cur"))
            return 0
            ;;

        # workout → log
        workout)
            COMPREPLY=($(compgen -W "log" -- "$cur"))
            return 0
            ;;

        # bodyweight → log
        bodyweight)
            COMPREPLY=($(compgen -W "log" -- "$cur"))
            return 0
            ;;

        # sleep → log
        sleep)
            COMPREPLY=($(compgen -W "log" -- "$cur"))
            return 0
            ;;

        # nutrition → log
        nutrition)
            COMPREPLY=($(compgen -W "log" -- "$cur"))
            return 0
            ;;

        # aliases
        streaks)
            return 0
            ;;
        pr)
            return 0
            ;;
        activity)
            return 0
            ;;

    esac

    # ── level 3 — soma workout log [TAB] ────────────────
    # soma workout log → suggest exercise names
    if [ "$pprev" = "workout" ] && [ "$prev" = "log" ]; then
        COMPREPLY=($(compgen -W "$exercises" -- "$cur"))
        return 0
    fi

    # soma bodyweight log → no completion (expects a number)
    if [ "$pprev" = "bodyweight" ] && [ "$prev" = "log" ]; then
        return 0
    fi

    # soma sleep log → no completion (expects hours)
    if [ "$pprev" = "sleep" ] && [ "$prev" = "log" ]; then
        return 0
    fi

    # soma nutrition log → no completion (expects numbers)
    if [ "$pprev" = "nutrition" ] && [ "$prev" = "log" ]; then
        return 0
    fi
}

complete -F _soma_completion soma