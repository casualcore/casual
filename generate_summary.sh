#!/bin/sh
sed -i '/^<!-- summary below -->$/q' README.md
find . -name "*.md" -not -ipath "*unittest*" -not -ipath "./README.md" -printf "%d [%p](%p)\n" | sort -k 1,2 | awk '{print "* " $NF}' >> README.md

