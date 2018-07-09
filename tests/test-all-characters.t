#!/bin/sh
. ./common.sh

f_input="${base}.tmp"
f_expected="${base}.tmp"
perl -e 'use List::Util qw(shuffle); srand 0; print map(chr, shuffle 0..0xff)' > "$f_input"
run_shellcat

# vim:ft=sh ts=4 sts=4 sw=4 et
