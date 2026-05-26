CC = gcc
PKG_CONFIG ?= pkg-config
VALGRIND ?= valgrind
WINE ?= wine
CURL ?= curl
TAR ?= tar
WIN32_HOST ?= x86_64-w64-mingw32
WIN32_CC ?= $(WIN32_HOST)-gcc

BASE_CFLAGS ?= -O3 -g -std=c99 -Wall -Wextra -Werror -pedantic -DTRA_FFIC_TRACK_CLOSURES
CFLAGS ?= $(BASE_CFLAGS)
ASAN_CFLAGS ?= -O1 -g -std=c99 -Wall -Wextra -Werror -pedantic -DTRA_FFIC_TRACK_CLOSURES -fsanitize=address -fno-omit-frame-pointer
ASAN_OPTIONS ?= detect_leaks=1:halt_on_error=1
LIBFFI_CFLAGS := $(shell $(PKG_CONFIG) --cflags libffi)
LIBFFI_LIBS := $(shell $(PKG_CONFIG) --libs libffi)
WIN32_CFLAGS ?= -O3 -g -std=c99 -Wall -Wextra -Werror -DTRA_FFIC_TRACK_CLOSURES -DTRA_FFIC_IN_WIN32=1 -DTRA_FFIC_IN_POSIX=0 -D_WIN32_WINNT=0x0600
WIN32_LDFLAGS ?= -static-libgcc
LIBFFI_VERSION ?= 3.4.6
LIBFFI_PACKAGE := libffi-$(LIBFFI_VERSION).tar.gz
LIBFFI_URL := https://github.com/libffi/libffi/releases/download/v$(LIBFFI_VERSION)/$(LIBFFI_PACKAGE)

BUILD_DIR := .build
DEPS_DIR := $(BUILD_DIR)/deps
LIBFFI_ARCHIVE := $(DEPS_DIR)/$(LIBFFI_PACKAGE)
WIN32_LIBFFI_SOURCE_DIR := $(DEPS_DIR)/src/libffi-$(LIBFFI_VERSION)-win32
WIN32_LIBFFI_PREFIX := $(DEPS_DIR)/libffi-win32
WIN32_LIBFFI_PREFIX_ABS := $(abspath $(WIN32_LIBFFI_PREFIX))
WIN32_LIBFFI_STAMP := $(WIN32_LIBFFI_PREFIX)/.built-$(LIBFFI_VERSION)
WIN32_LIBFFI_CFLAGS := -I$(WIN32_LIBFFI_PREFIX)/include
WIN32_LIBFFI_LIBS := $(WIN32_LIBFFI_PREFIX)/lib/libffi.a
TEST_BIN := $(BUILD_DIR)/tra_ffic_test
TEST_ASAN_BIN := $(BUILD_DIR)/tra_ffic_test_asan
TEST_WIN32_BIN := $(BUILD_DIR)/tra_ffic_test_win32.exe

.PHONY: test test-all test-plain test-valgrind test-asan test-win32 libffi-win32 clean

test: test-plain
	$(MAKE) test-valgrind
	$(MAKE) test-asan

test-all:
	$(MAKE) test
	$(MAKE) test-win32

test-plain: $(TEST_BIN)
	$(TEST_BIN)

test-valgrind: $(TEST_BIN)
	$(VALGRIND) --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --error-exitcode=99 $(TEST_BIN)

test-asan: $(TEST_ASAN_BIN)
	ASAN_OPTIONS="$(ASAN_OPTIONS)" $(TEST_ASAN_BIN)

test-win32: $(TEST_WIN32_BIN)
	$(WINE) $(TEST_WIN32_BIN)

libffi-win32: $(WIN32_LIBFFI_STAMP)

$(TEST_BIN): tests/tra_ffic_test.c include/tra_ffic.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LIBFFI_CFLAGS) -Iinclude $< -o $@ $(LIBFFI_LIBS) -pthread -lm

$(TEST_ASAN_BIN): tests/tra_ffic_test.c include/tra_ffic.h | $(BUILD_DIR)
	$(CC) $(ASAN_CFLAGS) $(LIBFFI_CFLAGS) -Iinclude $< -o $@ $(LIBFFI_LIBS) -pthread -lm

$(TEST_WIN32_BIN): tests/tra_ffic_test.c include/tra_ffic.h $(WIN32_LIBFFI_STAMP) | $(BUILD_DIR)
	$(WIN32_CC) $(WIN32_CFLAGS) $(WIN32_LIBFFI_CFLAGS) -Iinclude $< -o $@ $(WIN32_LIBFFI_LIBS) $(WIN32_LDFLAGS) -lm

$(WIN32_LIBFFI_STAMP):
	command -v $(WIN32_CC) >/dev/null
	command -v $(CURL) >/dev/null
	command -v $(TAR) >/dev/null
	command -v make >/dev/null
	mkdir -p $(DEPS_DIR) $(DEPS_DIR)/src
	if [ ! -f "$(LIBFFI_ARCHIVE)" ]; then \
		$(CURL) -fL -o "$(LIBFFI_ARCHIVE)" "$(LIBFFI_URL)"; \
	fi
	rm -rf "$(WIN32_LIBFFI_SOURCE_DIR)" "$(WIN32_LIBFFI_PREFIX)"
	mkdir -p "$(WIN32_LIBFFI_SOURCE_DIR)"
	$(TAR) -xzf "$(LIBFFI_ARCHIVE)" -C "$(WIN32_LIBFFI_SOURCE_DIR)" --strip-components=1
	cd "$(WIN32_LIBFFI_SOURCE_DIR)" && ./configure --host="$(WIN32_HOST)" --prefix="$(WIN32_LIBFFI_PREFIX_ABS)" --disable-shared --enable-static --disable-docs
	$(MAKE) -C "$(WIN32_LIBFFI_SOURCE_DIR)" -j"$$(nproc)"
	$(MAKE) -C "$(WIN32_LIBFFI_SOURCE_DIR)" install
	touch "$(WIN32_LIBFFI_STAMP)"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
