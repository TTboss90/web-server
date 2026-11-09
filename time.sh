#!/usr/bin/env bash

url="${1:-https://example.com}"
count="${2:-2}"

tmpdir=$(mktemp -d)
trap 'rm -rf "$tmpdir"' EXIT

for ((i=1; i<=count; i++)); do
    curl -k -sS \
    -o /dev/null \
    -w '%{time_total}' \
    -X POST \
    -b 'test=hiandhello' \
    -d 'foo=bar&hello=world' \
    "https://$url" > "$tmpdir/$i.txt" &
done

wait

for ((i=1; i<=count; i++)); do
    echo "Curl $i: $(cat "$tmpdir/$i.txt")s"
done
