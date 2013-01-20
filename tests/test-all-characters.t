#!/bin/sh
. ./common.sh

f_input="${base}.tmp"
f_expected="${base}.tmp"
for a in 0 1 2 3
do
    for b in 0 1 2 3 4 5 6 7
    do
        for c in 0 1 2 3 4 5 6 7
        do
            printf '%s\n' "\\$a$b$c"
        done
    done
done \
| shuf --random-source="$0" \
| tr -d '\n' \
| xargs -0 printf \
> "$f_input"
run_shellcat

# vim:ft=sh ts=4 sw=4 et
