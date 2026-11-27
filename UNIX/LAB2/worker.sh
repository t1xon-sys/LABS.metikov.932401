#!/bin/sh
dir=${DATA_DIR:-/data}            
lock=$dir/.lock                   

id=$(od -An -N4 -tx1 /dev/urandom | tr -d ' \n')   
n=0
file=

trap '[ -n "$file" ] && rm -f "$file"; exit 0' INT TERM HUP

exec 9>>"$lock"                  

while :; do
    n=$((n + 1))

    flock -x 9                    
    i=1
    while [ -e "$dir/$(printf '%03d' "$i")" ]; do
        i=$((i + 1))
    done
    file=$dir/$(printf '%03d' "$i")
    printf '%s %d\n' "$id" "$n" > "$file"
    flock -u 9                    
    echo "$id: создан $file ($n)"
    sleep 1

    [ "$(cat "$file" 2>/dev/null)" = "$id $n" ] || echo "$id: RACE в $file" >&2
    rm -f "$file"
    file=
    sleep 1
done