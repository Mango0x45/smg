#!/usr/bin/env sh

# RUN WITH `make test`

BIN=../smg

cd "$(dirname "$0")"

for t in $(ls * | grep -v '\.'); do
	VOUT=$(valgrind -s $BIN $t 2>&1 |
		sed -En 's/==[0-9]+==\s+(.* lost: [1-9][0-9]* bytes in [1-9][0-9]* blocks)/\1/p')
	[ -n "$VOUT" ] && printf '==> %s <==\n%s\n' $t "$VOUT"
	$BIN $t | man -P less /dev/stdin
done
