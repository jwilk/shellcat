set -e -u

shellcat=${shellcat:-../shellcat}
base="${0%.t}"
f_input="${base}.in"
f_expected="${base}.exp"
f_output="${base}.out"

run_shellcat()
{
    "$shellcat" "$f_input" "$@" > "$f_output"
    exec diff -u "$f_expected" "$f_output" 
}

# vim:ts=4 sw=4 et
