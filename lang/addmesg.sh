#!/bin/bash

clear
echo ".: starting script \`addmesg'"
for F in *.inc.c
do
    echo -e ".: adding message to file \`$F'"
    echo -e "char* lstr$1 = \"$2\"; /*[!] to translate */" >> $F
    sort $F -o $F
done
echo ".: done!"