#!/bin/sh
set -eu

objdump=$1
binary=$2
expected="$3 $4 $5"
headers=$("$objdump" --macho --private-headers "$binary")
actual=$(printf '%s\n' "$headers" | awk '
    $1 == "MH_MAGIC_64" && $5 == "EXECUTE" { arch = tolower($2) }
    $1 == "cmd" { command = $2 }
    command == "LC_VERSION_MIN_MACOSX" && $1 == "version" { platform = "macos"; minimum = $2 }
    command == "LC_BUILD_VERSION" && $1 == "platform" { platform = tolower($2); gsub(/[-_]/, "", platform) }
    command == "LC_BUILD_VERSION" && $1 == "minos" { minimum = $2 }
    END { print arch, platform, minimum }
')
if [ "$actual" != "$expected" ]; then
    echo "$binary: expected '$expected', found '$actual'" >&2
    exit 1
fi
echo "$binary: $actual"
