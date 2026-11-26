#!/bin/sh
set -eu

source_dir=$1
destination=$2
test -f "$source_dir/configure.ac" || {
    echo 'Initialize libffi with: git submodule update --init deps/libffi' >&2
    exit 1
}
parent=$(dirname "$destination")
name=$(basename "$destination")
mkdir -p "$parent"
stage=$(mktemp -d "$parent/.prepare-XXXXXX")
trap 'rm -rf "$stage"' EXIT
trap 'exit 1' HUP INT TERM
mkdir "$stage/$name"
git -C "$source_dir" archive -o "$stage/source.tar" HEAD
tar -xf "$stage/source.tar" -C "$stage/$name"
if [ "$(uname -s)" = Darwin ]; then
    export LIBTOOLIZE="${LIBTOOLIZE:-glibtoolize}"
fi
# libffi 3.8 emits an unconditional Autoconf obsolescence notice for STDC_HEADERS.
(cd "$stage/$name" && WARNINGS=no-obsolete ./autogen.sh)
touch "$stage/$name/.prepared"
# Concurrent builds can publish the same prepared revision.
mv "$stage/$name" "$parent/" || test -f "$destination/.prepared"
