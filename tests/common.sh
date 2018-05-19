set -e -u

shellcat=${shellcat:-../shellcat}
shellcat_ifs=${shellcat_ifs:-$IFS}
ifs=$IFS
base="${0%.t}"
f_input="${base}.in"
f_expected="${base}.xout"
f_exp_err="${base}.xerr"
[ -e "$f_exp_err" ] || f_exp_err=/dev/null
f_output="${base}.out"
f_errors="${base}.err"

run_shellcat()
{
    IFS="$shellcat_ifs"
    set -- $shellcat "$f_input" "$@"
    IFS="$ifs"
    "$@" > "$f_output" 2>"$f_errors"
    rc=$?
    diff -u "$f_expected" "$f_output" &&
    diff -u "$f_exp_err" "$f_errors" &&
    return $rc
}

# vim:ts=4 sts=4 sw=4 et
