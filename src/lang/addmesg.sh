#!/bin/sh
clear
echo ".: starting script \`addmesg'"
for F in *.inc.h
do
    echo -e ".: adding message to file \`$F'"
    echo -e "char* lstr$1 = \"$2\"; /*[!] to translate */" >> $F
    sort $F -o $F
done
echo ".: done!"