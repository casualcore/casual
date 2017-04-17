#!/bin/sh
find . -depth -type d -print | tac | xargs -I % bash -c "ls -1 %/*.md 2>/dev/null" | xargs -I % printf "[%](%)\n\n" > index.md

