#!/bin/sh
< ./CHANGELOG.m grep -v '<items>' | sed -e \
"s/<date>//g
 s/<\/date>//g
 s/<items>//g
 s/<\/items>//g
 s/<item>/	* /g
 s/<\/item>//g
 s/<hl>/-= /g
 s/<\/hl>/ =-/g
 s/<hf>/\`/g
 s/<\/hf>/\'/g
 s/<hc>/\`/g
 s/<\/hc>/\'/g
 s/&lt;/</g
 s/&gt;/>/g"
