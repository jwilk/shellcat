#!/bin/sh
. ./common.sh

f_input="${base}.tmp"
f_expected="${base}.tmp"
yes | head -n 1M > "$f_input"
run_shellcat

# vim:ts=4 sw=4 et
