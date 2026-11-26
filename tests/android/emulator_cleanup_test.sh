#!/bin/sh

set -eu

runner=$(dirname "$0")/run_emulator_tests.sh
test_root=$(mktemp -d)
cleanup() {
    if [ -s "$test_root/emulator.pid" ]; then
        kill -KILL "$(cat "$test_root/emulator.pid")" 2>/dev/null || true
    fi
    rm -rf -- "$test_root"
}
trap cleanup EXIT

export CLEANUP_TEST_ROOT="$test_root" TMPDIR="$test_root/tmp"
export ANDROID_SDK_ROOT="$test_root/sdk" ANDROID_NDK_ROOT="$test_root/ndk"
export ANDROID_SDK_MANAGER=/bin/true ANDROID_AVD_MANAGER=/bin/true
export ANDROID_EMULATOR="$test_root/bin/emulator" ADB="$test_root/bin/adb"
export PATH="$test_root/bin:$PATH"
toolchain="$ANDROID_NDK_ROOT/toolchains/llvm/prebuilt/linux-x86_64/bin"
image="$ANDROID_SDK_ROOT/system-images/android-35/google_apis_ps16k/x86_64"
mkdir -p "$test_root/bin" "$TMPDIR" "$toolchain" "$image"
ln -s /bin/true "$toolchain/x86_64-linux-android24-clang"
touch "$image/package.xml"
mkfifo "$test_root/ready"

cat > "$ANDROID_EMULATOR" <<'EOF'
#!/bin/sh
trap '' TERM
echo "$$" > "$CLEANUP_TEST_ROOT/ready"
exec sleep infinity
EOF
cat > "$ADB" <<'EOF'
#!/bin/sh
case "$3" in
    wait-for-device) cat "$CLEANUP_TEST_ROOT/ready" > "$CLEANUP_TEST_ROOT/emulator.pid" ;;
    shell) echo 1 ;;
    emu)
        if [ "$CLEANUP_TEST_STATUS" -ne 0 ]; then exec sleep infinity; fi
        ;;
esac
EOF
cat > "$test_root/bin/make" <<'EOF'
#!/bin/sh
exit "$CLEANUP_TEST_STATUS"
EOF
chmod +x "$ANDROID_EMULATOR" "$ADB" "$test_root/bin/make"

# Exercise both an acknowledged shutdown and an unresponsive console.
for CLEANUP_TEST_STATUS in 0 23; do
    export CLEANUP_TEST_STATUS
    status=0
    timeout --kill-after=3s 30s "$runner" 35 google_apis_ps16k 16384 || status=$?
    if [ "$status" -ne "$CLEANUP_TEST_STATUS" ]; then
        echo "Expected test status $CLEANUP_TEST_STATUS after shutdown, got $status" >&2
        exit 1
    fi
    if kill -0 "$(cat "$test_root/emulator.pid")" 2>/dev/null; then
        echo "The emulator is still running after the tests" >&2
        exit 1
    fi
    rm "$test_root/emulator.pid"
    test -z "$(ls -A "$TMPDIR")"
done
echo "emulator_cleanup_test: PASS"
