#!/bin/sh
. ${0%/*}/common.sh

f_input="${base}.tmp"
printf 'OK\n<$ exit 0 $>' > "$f_input"
truncate -s 4G "$f_input"
run_shellcat 141
rm "$f_input"

# vim:ft=sh ts=4 sts=4 sw=4 et
