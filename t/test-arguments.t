#!/bin/sh
. ${0%/*}/common.sh

run_shellcat 0 eggs "'bacon'" '
s
p
a
m
'
# vim:ft=sh ts=4 sts=4 sw=4 et
