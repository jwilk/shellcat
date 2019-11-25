#!/bin/sh
. ${0%/*}/common.sh

f_input="${base}.tmp"
f_expected="${base}.tmp"
yes | head -n 1000000 > "$f_input"
run_shellcat
rm "$f_input"

# vim:ft=sh ts=4 sts=4 sw=4 et
