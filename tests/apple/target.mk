# Build one SDK, CPU and deployment target per invocation.
.DELETE_ON_ERROR:
CPU := $(patsubst arm64,aarch64,$(ARCH))
MINIMUM := $(if $(filter x86_64,$(ARCH)),10.13,11.0)
MACHO_PLATFORM := macos
ifeq ($(PLATFORM),macos-cross)
# The Darwin libSystem stubs shipped by Zig 0.15.2 target SDK 15.5.
SDK_VERSION := 15.5
DARWIN_STUBS := $(ZIG_DIR)/lib/libc/darwin
TARGET := $(CPU)-macos.$(MINIMUM)
FFI_HOST := $(CPU)-apple-darwin
APPLE_CC := $(ZIG_DIR)/zig cc -target $(TARGET)
APPLE_CXX := $(ZIG_DIR)/zig c++ -target $(TARGET)
NDK_LLVM := $(ANDROID_NDK_ROOT)/toolchains/llvm/prebuilt/linux-x86_64/bin
LLVM_BIN ?= $(if $(wildcard $(NDK_LLVM)/ld64.lld),$(NDK_LLVM),$(shell command -v llvm-config >/dev/null && llvm-config --bindir))
LLVM_PREFIX := $(if $(LLVM_BIN),$(LLVM_BIN)/)
APPLE_AR := $(LLVM_PREFIX)llvm-ar
APPLE_RANLIB := $(LLVM_PREFIX)llvm-ranlib
APPLE_NM := $(LLVM_PREFIX)llvm-nm
APPLE_STRIP := $(LLVM_PREFIX)llvm-strip
APPLE_LD := $(LLVM_PREFIX)ld64.lld
APPLE_OBJDUMP := $(LLVM_PREFIX)llvm-objdump
APPLE_DSYMUTIL := $(LLVM_PREFIX)dsymutil
else
XCRUN := xcrun --sdk $(PLATFORM)
SDK_VERSION := $(shell $(XCRUN) --show-sdk-version)
MINIMUM := $(or $(MACOSX_DEPLOYMENT_TARGET),$(MINIMUM))
TARGET := $(ARCH)-apple-macos$(MINIMUM)
FFI_HOST := $(shell "$(FFI_SOURCE)/config.guess")
APPLE_CC := $(XCRUN) clang -target $(TARGET)
APPLE_CXX := $(XCRUN) clang++ -target $(TARGET)
APPLE_AR := $(shell $(XCRUN) --find ar)
APPLE_RANLIB := $(shell $(XCRUN) --find ranlib)
APPLE_NM := $(shell $(XCRUN) --find nm)
APPLE_STRIP := $(shell $(XCRUN) --find strip)
APPLE_LD := $(shell $(XCRUN) --find ld)
APPLE_OBJDUMP := $(shell $(XCRUN) --find llvm-objdump)
APPLE_DSYMUTIL := $(shell $(XCRUN) --find dsymutil)
endif

OUT := $(OUTPUT_ROOT)/$(PLATFORM)-$(SDK_VERSION)/$(ARCH)-$(MINIMUM)
FFI_BUILD := $(OUT)/libffi
ARCHIVE := $(FFI_BUILD)/.libs/libffi.a
TEST := $(OUT)/tra_ffic_test
TEST_FLAGS := -O3 -g -std=c99 -Wall -Wextra -Werror -pedantic -DTRA_FFIC_TRACK_CLOSURES

$(FFI_BUILD)/.configured: $(FFI_SOURCE)/.prepared tests/apple/target.mk
	rm -rf "$(FFI_BUILD)"
	mkdir -p "$(FFI_BUILD)"
	# tra-ffic uses the C closure API. Omit libffi's deprecated Java raw interface.
	cd "$(FFI_BUILD)" && CC='$(APPLE_CC)' CXX='$(APPLE_CXX)' \
		AR="$(APPLE_AR)" RANLIB="$(APPLE_RANLIB)" NM="$(APPLE_NM)" STRIP="$(APPLE_STRIP)" \
		LD="$(APPLE_LD)" OBJDUMP="$(APPLE_OBJDUMP)" DSYMUTIL="$(APPLE_DSYMUTIL)" \
		FILECMD=file MANIFEST_TOOL=: READELF=: \
		CPPFLAGS=-DNO_JAVA_RAW_API CFLAGS=-O2 \
		"$(FFI_SOURCE)/configure" --build="$$("$(FFI_SOURCE)/config.guess")" \
		--host="$(FFI_HOST)" --disable-shared --enable-static --disable-docs \
		> configure-output.log 2>&1 || { cat "$(FFI_BUILD)/configure-output.log" >&2; exit 1; }
	touch "$@"

$(ARCHIVE): $(FFI_BUILD)/.configured
	$(MAKE) -C "$(FFI_BUILD)" libffi.la

$(OUT)/regression.o: tests/tra_ffic_test.c include/tra_ffic.h $(FFI_BUILD)/.configured
	$(APPLE_CC) $(TEST_FLAGS) -isystem "$(FFI_BUILD)/include" -Iinclude -c "$<" -o "$@"

$(TEST): $(OUT)/regression.o $(ARCHIVE)
ifeq ($(PLATFORM),macos-cross)
	"$(APPLE_LD)" -fatal_warnings -arch "$(ARCH)" -platform_version macos $(MINIMUM) $(SDK_VERSION) \
		-L"$(DARWIN_STUBS)" -o "$@" "$<" "$(ARCHIVE)" -lSystem
else
	$(APPLE_CC) -Wl,-fatal_warnings -o "$@" "$<" "$(ARCHIVE)"
endif

.PHONY: verify run-macos
verify: $(TEST)
	sh tests/apple/check-macho.sh "$(APPLE_OBJDUMP)" "$(TEST)" "$(ARCH)" "$(MACHO_PLATFORM)" "$(MINIMUM)"

run-macos: verify
	"$(TEST)"
	@echo "tra_ffic_macos_test: PASS ($(ARCH), deployment $(MINIMUM), macOS $$(sw_vers -productVersion))"
