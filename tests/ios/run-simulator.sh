#!/bin/sh
set -eu

app=$1
log=$2
codesign --force --sign - "$app"
runtime=com.apple.CoreSimulator.SimRuntime.iOS-18-5
device=$(xcrun simctl create TraFficTest com.apple.CoreSimulator.SimDeviceType.iPhone-16 "$runtime")
cleanup() {
    xcrun simctl shutdown "$device" >/dev/null 2>&1 || true
    xcrun simctl delete "$device"
}
trap cleanup EXIT
trap 'exit 1' HUP INT TERM
xcrun simctl bootstatus "$device" -b
xcrun simctl install "$device" "$app"
if ! xcrun simctl launch --console "$device" com.example.tra-ffic.regression > "$log" 2>&1; then
    cat "$log" >&2
    exit 1
fi
cat "$log"
grep -Fxq 'tra_ffic_ios_test: PASS' "$log"
