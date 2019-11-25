set -e -u

shellcat=${shellcat:-${0%/*}/../shellcat}
shellcat_ifs=${shellcat_ifs:-$IFS}
ifs=$IFS
base="${0%.t}"
f_input="${base}.in"
f_expected="${base}.xout"
f_exp_err="${base}.xerr"
[ -e "$f_exp_err" ] || f_exp_err=/dev/null
f_output="${base}.out"
f_errors="${base}.err"

diag()
{
    sed -e 's/^/# /' <<EOF
$@
EOF
}

run_shellcat()
{
    echo 1..1
    IFS="$shellcat_ifs"
    if [ $# = 0 ]
    then
        xrc=0
    else
        xrc=$1
        shift
    fi
    set -- $shellcat "$f_input" "$@"
    IFS="$ifs"
    rc=0
    "$@" > "$f_output" 2>"$f_errors" || rc=$?
    outdiff=$(diff -u "$f_expected" "$f_output") || true
    errdiff=$(diff -u "$f_exp_err" "$f_errors") || true
    if [ -n "$errdiff" ]
    then
        echo not ok 1 unexpected stderr
        diag "$errdiff"
    elif [ -n "$outdiff" ]
    then
        echo not ok 1 unexpected stdout
        diag "$outdiff"
    elif [ $rc != $xrc ]
    then
        echo not ok 1 "\$? = $rc, expected $xrc)"
    else
        echo ok 1
    fi
    rm "$f_output" "$f_errors"
}

# vim:ts=4 sts=4 sw=4 et
