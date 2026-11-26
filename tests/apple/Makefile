APPLE_DIR := $(abspath $(BUILD_DIR)/apple)
APPLE_FFI_SOURCE := $(APPLE_DIR)/src/libffi-$(LIBFFI_REVISION)
APPLE_ZIG_DIR ?= $(abspath $(BUILD_DIR)/tools/zig-0.15.2)
APPLE_MAKE = $(MAKE) --no-print-directory -f tests/apple/target.mk \
	OUTPUT_ROOT="$(APPLE_DIR)/$(LIBFFI_REVISION)" FFI_SOURCE="$(APPLE_FFI_SOURCE)"

$(APPLE_FFI_SOURCE)/.prepared:
	sh tests/apple/prepare-libffi.sh "$(LIBFFI_SOURCE_DIR)" "$(APPLE_FFI_SOURCE)"

$(APPLE_ZIG_DIR)/zig:
	sh tests/apple/install-zig.sh "$(APPLE_ZIG_DIR)"

.PHONY: macos-cross macos-cross-x86_64 macos-cross-arm64
macos-cross: macos-cross-x86_64 macos-cross-arm64

macos-cross-x86_64 macos-cross-arm64: $(APPLE_FFI_SOURCE)/.prepared | $(APPLE_ZIG_DIR)/zig
	+$(APPLE_MAKE) PLATFORM=macos-cross ARCH=$(@:macos-cross-%=%) \
		ZIG_DIR="$(APPLE_ZIG_DIR)" verify
