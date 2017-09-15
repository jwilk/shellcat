#!/bin/sh
. ./common.sh

f_input="${base}.tmp"
printf 'OK\n<$ exit 0 $>' > "$f_input"
truncate -s 4G "$f_input"
run_shellcat || [ $? -eq 141 ] || exit 1

# vim:ft=sh ts=4 sts=4 sw=4 et
