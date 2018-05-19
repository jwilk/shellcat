#!/bin/sh
. ./common.sh
f_input=/dev/null
f_expected=/dev/null
rc=0
run_shellcat -s true || rc=$?
[ $rc -eq 1 ]

# vim:ft=sh ts=4 sts=4 sw=4 et
