#!/usr/bin/env bash

set -e

PREFIX=/usr/local
BIN_DIR=$PREFIX/bin
name='ash'
greeting="hello, world"

greet() {
    printf '%s\n' "$greeting"
}

count_args() {
    local n=$1
    echo "args: $n"
}

if [ -d "$BIN_DIR" ]; then
    echo "found $BIN_DIR"
elif [ -d /usr/bin ]; then
    echo "falling back to /usr/bin"
else
    echo "no bin dir" 1>&2
fi

for f in a.txt b.txt c.txt; do
    echo "processing $f"
done

i=0
while [ $i -lt 3 ]; do
    i=$(( i + 1 ))
    echo "i=$i"
done

until [ $i -eq 0 ]; do
    i=$(( i - 1 ))
done

case "$name" in
    (ash) echo "this is ash" ;;
    (bash|sh) echo "close enough" ;;
    (*) echo "unknown" ;;
esac

today=$(date +%Y-%m-%d)
echo "today is ${today}"

ls -la /tmp | grep -v '^total' | wc -l > /dev/null

{ echo one; echo two; } > /dev/null 2>&1

( cd /tmp && ls ) > /dev/null

greet && count_args 3 || echo "failed"

echo "${name}-${today}" > /dev/null

function usage {
    echo "usage: $0 [options]" >&2
}

show_all() {
    for arg; do
        printf '%s\n' "$arg"
    done
}

target=${DESTDIR:-/opt}/share
echo ${name}-${today} > /dev/null
echo "$target" >> /dev/null
echo $? > /dev/null
wc -l < /etc/hostname > /dev/null

! false && echo ok

sleep 0 &

exit 0
