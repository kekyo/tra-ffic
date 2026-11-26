#!/bin/sh
set -eu

destination=$1
parent=$(dirname "$destination")
mkdir -p "$parent"
stage=$(mktemp -d "$parent/.zig-XXXXXX")
trap 'rm -rf "$stage"' EXIT
trap 'exit 1' HUP INT TERM

archive=zig-x86_64-linux-0.15.2
curl --fail --location --silent --show-error --retry 3 \
    "https://ziglang.org/download/0.15.2/$archive.tar.xz" -o "$stage/zig.tar.xz"
printf '%s  %s\n' 02aa270f183da276e5b5920b1dac44a63f1a49e55050ebde3aecc9eb82f93239 \
    "$stage/zig.tar.xz" | sha256sum -c -
tar --no-same-owner -xJf "$stage/zig.tar.xz" -C "$stage"
test -x "$stage/$archive/zig"
mv -T "$stage/$archive" "$destination" || test -x "$destination/zig"
