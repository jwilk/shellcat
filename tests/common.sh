set -e -u

shellcat=${shellcat:-../shellcat}
shellcat_ifs=${shellcat_ifs:-$IFS}
ifs=$IFS
base="${0%.t}"
f_input="${base}.in"
f_expected="${base}.exp"
f_output="${base}.out"

run_shellcat()
{
    IFS="$shellcat_ifs"
    set -- $shellcat "$f_input" "$@"
    IFS="$ifs"
    "$@" > "$f_output"
    exec diff -u "$f_expected" "$f_output"
}

# vim:ts=4 sw=4 et
