CC = gcc
PKG_CONFIG ?= pkg-config
VALGRIND ?= valgrind
WINE ?= wine
WINEBOOT ?= wineboot
WINE_ARCH ?= win64
WINE_DEBUG ?= -all
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
ANDROID_NDK_ROOT ?=
ANDROID_SDK_ROOT ?=
ANDROID_API ?= 24
ANDROID_HOST_TAG ?= linux-x86_64
ANDROID_BUILD_TOOLS_VERSION ?= 36.0.0
ANDROID_PLATFORM_VERSION ?= 37.0
ANDROID_TOOLCHAIN := $(ANDROID_NDK_ROOT)/toolchains/llvm/prebuilt/$(ANDROID_HOST_TAG)/bin
ANDROID_BUILD_TOOLS := $(ANDROID_SDK_ROOT)/build-tools/$(ANDROID_BUILD_TOOLS_VERSION)
ANDROID_PLATFORM_JAR := $(ANDROID_SDK_ROOT)/platforms/android-$(ANDROID_PLATFORM_VERSION)/android.jar
ANDROID_X86_64_HOST ?= x86_64-linux-android
ANDROID_ARM64_HOST ?= aarch64-linux-android
ANDROID_X86_64_CC ?= $(ANDROID_TOOLCHAIN)/x86_64-linux-android$(ANDROID_API)-clang
ANDROID_X86_64_CXX ?= $(ANDROID_TOOLCHAIN)/x86_64-linux-android$(ANDROID_API)-clang++
ANDROID_ARM64_CC ?= $(ANDROID_TOOLCHAIN)/aarch64-linux-android$(ANDROID_API)-clang
ANDROID_ARM64_CXX ?= $(ANDROID_TOOLCHAIN)/aarch64-linux-android$(ANDROID_API)-clang++
ANDROID_AR ?= $(ANDROID_TOOLCHAIN)/llvm-ar
ANDROID_RANLIB ?= $(ANDROID_TOOLCHAIN)/llvm-ranlib
ANDROID_STRIP ?= $(ANDROID_TOOLCHAIN)/llvm-strip
ANDROID_READELF ?= $(ANDROID_TOOLCHAIN)/llvm-readelf
ANDROID_CFLAGS ?= -O3 -g -std=c99 -Wall -Wextra -Werror -pedantic -DTRA_FFIC_TRACK_CLOSURES -fPIC
ANDROID_LIBFFI_CFLAGS ?= -O3 -g -fPIC
ANDROID_LDFLAGS ?= -shared -Wl,--no-undefined -Wl,-z,max-page-size=16384
ANDROID_LIBFFI_LDFLAGS ?= -Wl,-z,max-page-size=16384
ANDROID_LDLIBS ?= -pthread -ldl -lm
ANDROID_AAPT2 ?= $(ANDROID_BUILD_TOOLS)/aapt2
ANDROID_D8 ?= $(ANDROID_BUILD_TOOLS)/d8
ANDROID_ZIPALIGN ?= $(ANDROID_BUILD_TOOLS)/zipalign
ANDROID_APKSIGNER ?= $(ANDROID_BUILD_TOOLS)/apksigner
JAVAC ?= javac
KEYTOOL ?= keytool
ZIP ?= zip
UNZIP ?= unzip
ADB ?= adb
ADB_SERIAL ?=
ANDROID_RUNTIME_ABI ?= x86_64
ANDROID_EXPECTED_API ?=
ANDROID_EXPECTED_PAGE_SIZE ?=
ANDROID_ADB := $(ADB) $(if $(strip $(ADB_SERIAL)),-s $(ADB_SERIAL),)

BUILD_DIR := .build
WINE_PREFIX ?= $(abspath $(BUILD_DIR)/wine-prefix)
WINE_PREFIX_STAMP := $(WINE_PREFIX)/.tra-ffic-ready
DEPS_DIR := $(BUILD_DIR)/deps
LIBFFI_SOURCE_DIR := deps/libffi
WIN32_LIBFFI_BUILD_DIR := $(DEPS_DIR)/src/libffi-win32
WIN32_LIBFFI_PREFIX := $(DEPS_DIR)/libffi-win32
WIN32_LIBFFI_PREFIX_ABS := $(abspath $(WIN32_LIBFFI_PREFIX))
LIBFFI_REVISION := $(shell git -C $(LIBFFI_SOURCE_DIR) rev-parse --short=12 HEAD 2>/dev/null || printf uninitialized)
WIN32_LIBFFI_STAMP := $(WIN32_LIBFFI_PREFIX)/.built-$(LIBFFI_REVISION)
WIN32_LIBFFI_CFLAGS := -I$(WIN32_LIBFFI_PREFIX)/include
WIN32_LIBFFI_LIBS := $(WIN32_LIBFFI_PREFIX)/lib/libffi.a
TEST_BIN := $(BUILD_DIR)/tra_ffic_test
TEST_ASAN_BIN := $(BUILD_DIR)/tra_ffic_test_asan
TEST_WIN32_BIN := $(BUILD_DIR)/tra_ffic_test_win32.exe
ANDROID_BUILD_DIR := $(BUILD_DIR)/android
ANDROID_X86_64_BUILD_DIR := $(ANDROID_BUILD_DIR)/x86_64
ANDROID_ARM64_BUILD_DIR := $(ANDROID_BUILD_DIR)/arm64-v8a
ANDROID_X86_64_LIBFFI_BUILD_DIR := $(DEPS_DIR)/src/libffi-android-x86_64
ANDROID_ARM64_LIBFFI_BUILD_DIR := $(DEPS_DIR)/src/libffi-android-arm64-v8a
ANDROID_X86_64_LIBFFI_PREFIX := $(DEPS_DIR)/libffi-android-x86_64
ANDROID_ARM64_LIBFFI_PREFIX := $(DEPS_DIR)/libffi-android-arm64-v8a
ANDROID_X86_64_LIBFFI_PREFIX_ABS := $(abspath $(ANDROID_X86_64_LIBFFI_PREFIX))
ANDROID_ARM64_LIBFFI_PREFIX_ABS := $(abspath $(ANDROID_ARM64_LIBFFI_PREFIX))
ANDROID_X86_64_LIBFFI_STAMP := $(ANDROID_X86_64_LIBFFI_PREFIX)/.built-$(LIBFFI_REVISION)
ANDROID_ARM64_LIBFFI_STAMP := $(ANDROID_ARM64_LIBFFI_PREFIX)/.built-$(LIBFFI_REVISION)
ANDROID_X86_64_LIBFFI_CFLAGS := -isystem $(ANDROID_X86_64_LIBFFI_PREFIX)/include
ANDROID_ARM64_LIBFFI_CFLAGS := -isystem $(ANDROID_ARM64_LIBFFI_PREFIX)/include
ANDROID_X86_64_LIBFFI_LIBS := $(ANDROID_X86_64_LIBFFI_PREFIX)/lib/libffi.a
ANDROID_ARM64_LIBFFI_LIBS := $(ANDROID_ARM64_LIBFFI_PREFIX)/lib/libffi.a
ANDROID_TEST_SRC := tests/tra_ffic_test.c
ANDROID_JNI_TEST_SRC := tests/android/tra_ffic_android_test.c
ANDROID_X86_64_TEST_OBJ := $(ANDROID_X86_64_BUILD_DIR)/tra_ffic_test.o
ANDROID_ARM64_TEST_OBJ := $(ANDROID_ARM64_BUILD_DIR)/tra_ffic_test.o
ANDROID_X86_64_JNI_TEST_OBJ := $(ANDROID_X86_64_BUILD_DIR)/tra_ffic_android_test.o
ANDROID_ARM64_JNI_TEST_OBJ := $(ANDROID_ARM64_BUILD_DIR)/tra_ffic_android_test.o
ANDROID_X86_64_TEST_LIB := $(ANDROID_X86_64_BUILD_DIR)/libtra_ffic_android_test.so
ANDROID_ARM64_TEST_LIB := $(ANDROID_ARM64_BUILD_DIR)/libtra_ffic_android_test.so
ANDROID_APP_SRC_DIR := tests/android/app
ANDROID_APP_MANIFEST := $(ANDROID_APP_SRC_DIR)/AndroidManifest.xml
ANDROID_APP_JAVA_SRCS := $(ANDROID_APP_SRC_DIR)/src/com/example/traffic/TraFficInstrumentation.java
ANDROID_APP_BUILD_DIR := $(ANDROID_BUILD_DIR)/app
ANDROID_APP_CLASSES_DIR := $(ANDROID_APP_BUILD_DIR)/classes
ANDROID_APP_CLASSES_STAMP := $(ANDROID_APP_CLASSES_DIR)/.stamp
ANDROID_APP_DEX_DIR := $(ANDROID_APP_BUILD_DIR)/dex
ANDROID_APP_DEX := $(ANDROID_APP_DEX_DIR)/classes.dex
ANDROID_APP_KEYSTORE := $(ANDROID_APP_BUILD_DIR)/debug.keystore
ANDROID_X86_64_APP_STAGING_DIR := $(ANDROID_X86_64_BUILD_DIR)/app-staging
ANDROID_ARM64_APP_STAGING_DIR := $(ANDROID_ARM64_BUILD_DIR)/app-staging
ANDROID_X86_64_TEST_APK := $(ANDROID_X86_64_BUILD_DIR)/tra_ffic_android_test.apk
ANDROID_ARM64_TEST_APK := $(ANDROID_ARM64_BUILD_DIR)/tra_ffic_android_test.apk
ifeq ($(ANDROID_RUNTIME_ABI),x86_64)
ANDROID_RUNTIME_TEST_APK := $(ANDROID_X86_64_TEST_APK)
else ifeq ($(ANDROID_RUNTIME_ABI),arm64-v8a)
ANDROID_RUNTIME_TEST_APK := $(ANDROID_ARM64_TEST_APK)
else
$(error Unsupported ANDROID_RUNTIME_ABI: $(ANDROID_RUNTIME_ABI))
endif

.PHONY: test test-all test-plain test-valgrind test-asan test-win32 libffi-win32 test-android test-android-artifacts test-android-runtime clean

test: test-plain
	$(MAKE) test-valgrind
	$(MAKE) test-asan

test-all:
	$(MAKE) test
	$(MAKE) CURL=false TAR=false test-win32

test-plain: $(TEST_BIN)
	$(TEST_BIN)

test-valgrind: $(TEST_BIN)
	$(VALGRIND) --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --error-exitcode=99 $(TEST_BIN)

test-asan: $(TEST_ASAN_BIN)
	ASAN_OPTIONS="$(ASAN_OPTIONS)" $(TEST_ASAN_BIN)

test-win32: $(TEST_WIN32_BIN) $(WINE_PREFIX_STAMP)
	DISPLAY= WINEARCH="$(WINE_ARCH)" WINEDEBUG="$(WINE_DEBUG)" WINEPREFIX="$(WINE_PREFIX)" $(WINE) $(TEST_WIN32_BIN)

libffi-win32: $(WIN32_LIBFFI_STAMP)

test-android: test-android-artifacts

test-android-artifacts: $(ANDROID_X86_64_TEST_APK) $(ANDROID_ARM64_TEST_APK)
	@for binary in $(ANDROID_X86_64_TEST_LIB) $(ANDROID_ARM64_TEST_LIB); do \
		alignments="$$( $(ANDROID_READELF) -lW $$binary | awk '$$1 == "LOAD" { print $$NF }' | sort -u )"; \
		if [ "$$alignments" != "0x4000" ]; then \
			echo "Android ELF alignment mismatch: $$binary ($$alignments)"; exit 1; \
		fi; \
	done
	$(ANDROID_ZIPALIGN) -c -P 16 4 $(ANDROID_X86_64_TEST_APK)
	$(ANDROID_ZIPALIGN) -c -P 16 4 $(ANDROID_ARM64_TEST_APK)
	@$(ANDROID_AAPT2) dump badging $(ANDROID_X86_64_TEST_APK) | grep -Fqx "minSdkVersion:'24'"
	@$(ANDROID_AAPT2) dump badging $(ANDROID_ARM64_TEST_APK) | grep -Fqx "minSdkVersion:'24'"
	@$(UNZIP) -Z1 $(ANDROID_X86_64_TEST_APK) | grep -Fqx "lib/x86_64/libtra_ffic_android_test.so"
	@$(UNZIP) -Z1 $(ANDROID_ARM64_TEST_APK) | grep -Fqx "lib/arm64-v8a/libtra_ffic_android_test.so"
	@echo "tra_ffic_android_artifacts_test: PASS"

test-android-runtime: test-android
	@if [ -z "$(ANDROID_EXPECTED_API)" ]; then \
		echo "ANDROID_EXPECTED_API is required"; exit 1; \
	fi
	@if [ -z "$(ANDROID_EXPECTED_PAGE_SIZE)" ]; then \
		echo "ANDROID_EXPECTED_PAGE_SIZE is required"; exit 1; \
	fi
	$(ANDROID_ADB) get-state
	$(ANDROID_ADB) install -r $(ANDROID_RUNTIME_TEST_APK)
	@$(ANDROID_ADB) logcat -c
	@output="$$( timeout -k 5s 120s $(ANDROID_ADB) shell am instrument -w \
		-e expectedApi "$(ANDROID_EXPECTED_API)" \
		-e expectedPageSize "$(ANDROID_EXPECTED_PAGE_SIZE)" \
		-e expectedAbi "$(ANDROID_RUNTIME_ABI)" \
		com.example.traffic/.TraFficInstrumentation 2>&1 )"; status=$$?; \
	printf '%s\n' "$$output"; \
	if [ $$status -ne 0 ] || ! printf '%s\n' "$$output" | grep -Fq "tra_ffic_android_test: PASS"; then \
		$(ANDROID_ADB) logcat -d -t 300 >&2 || true; \
		exit 1; \
	fi

# Initialize the isolated prefix separately so successful setup stays out of test logs.
$(WINE_PREFIX_STAMP): | $(BUILD_DIR)
	@wineboot_log="$(BUILD_DIR)/wineboot.log"; \
	if DISPLAY= WINEARCH="$(WINE_ARCH)" WINEDEBUG="$(WINE_DEBUG)" WINEPREFIX="$(WINE_PREFIX)" $(WINEBOOT) --init >"$$wineboot_log" 2>&1; then \
		rm -f "$$wineboot_log"; \
		touch "$@"; \
	else \
		status=$$?; \
		cat "$$wineboot_log" >&2; \
		rm -f "$$wineboot_log"; \
		exit $$status; \
	fi

$(TEST_BIN): tests/tra_ffic_test.c include/tra_ffic.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LIBFFI_CFLAGS) -Iinclude $< -o $@ $(LIBFFI_LIBS) -pthread -lm

$(TEST_ASAN_BIN): tests/tra_ffic_test.c include/tra_ffic.h | $(BUILD_DIR)
	$(CC) $(ASAN_CFLAGS) $(LIBFFI_CFLAGS) -Iinclude $< -o $@ $(LIBFFI_LIBS) -pthread -lm

$(TEST_WIN32_BIN): tests/tra_ffic_test.c include/tra_ffic.h $(WIN32_LIBFFI_STAMP) | $(BUILD_DIR)
	$(WIN32_CC) $(WIN32_CFLAGS) $(WIN32_LIBFFI_CFLAGS) -Iinclude $< -o $@ $(WIN32_LIBFFI_LIBS) $(WIN32_LDFLAGS) -lm

$(ANDROID_X86_64_TEST_OBJ): $(ANDROID_TEST_SRC) include/tra_ffic.h $(ANDROID_X86_64_LIBFFI_STAMP) | $(ANDROID_X86_64_BUILD_DIR)
	$(ANDROID_X86_64_CC) $(ANDROID_CFLAGS) $(ANDROID_X86_64_LIBFFI_CFLAGS) -Iinclude -Dmain=tra_ffic_android_regression_main -c $< -o $@

$(ANDROID_ARM64_TEST_OBJ): $(ANDROID_TEST_SRC) include/tra_ffic.h $(ANDROID_ARM64_LIBFFI_STAMP) | $(ANDROID_ARM64_BUILD_DIR)
	$(ANDROID_ARM64_CC) $(ANDROID_CFLAGS) $(ANDROID_ARM64_LIBFFI_CFLAGS) -Iinclude -Dmain=tra_ffic_android_regression_main -c $< -o $@

$(ANDROID_X86_64_JNI_TEST_OBJ): $(ANDROID_JNI_TEST_SRC) | $(ANDROID_X86_64_BUILD_DIR)
	$(ANDROID_X86_64_CC) $(ANDROID_CFLAGS) -c $< -o $@

$(ANDROID_ARM64_JNI_TEST_OBJ): $(ANDROID_JNI_TEST_SRC) | $(ANDROID_ARM64_BUILD_DIR)
	$(ANDROID_ARM64_CC) $(ANDROID_CFLAGS) -c $< -o $@

$(ANDROID_X86_64_TEST_LIB): $(ANDROID_X86_64_TEST_OBJ) $(ANDROID_X86_64_JNI_TEST_OBJ) $(ANDROID_X86_64_LIBFFI_STAMP)
	$(ANDROID_X86_64_CC) $(ANDROID_LDFLAGS) $(ANDROID_X86_64_TEST_OBJ) $(ANDROID_X86_64_JNI_TEST_OBJ) -o $@ $(ANDROID_X86_64_LIBFFI_LIBS) $(ANDROID_LDLIBS)

$(ANDROID_ARM64_TEST_LIB): $(ANDROID_ARM64_TEST_OBJ) $(ANDROID_ARM64_JNI_TEST_OBJ) $(ANDROID_ARM64_LIBFFI_STAMP)
	$(ANDROID_ARM64_CC) $(ANDROID_LDFLAGS) $(ANDROID_ARM64_TEST_OBJ) $(ANDROID_ARM64_JNI_TEST_OBJ) -o $@ $(ANDROID_ARM64_LIBFFI_LIBS) $(ANDROID_LDLIBS)

$(ANDROID_APP_CLASSES_STAMP): $(ANDROID_APP_JAVA_SRCS) $(ANDROID_APP_MANIFEST) Makefile
	mkdir -p $(ANDROID_APP_CLASSES_DIR)
	$(JAVAC) --release 8 -Xlint:-options -classpath $(ANDROID_PLATFORM_JAR) -d $(ANDROID_APP_CLASSES_DIR) $(ANDROID_APP_JAVA_SRCS)
	touch $@

$(ANDROID_APP_DEX): $(ANDROID_APP_CLASSES_STAMP)
	mkdir -p $(ANDROID_APP_DEX_DIR)
	$(ANDROID_D8) --min-api 24 --lib $(ANDROID_PLATFORM_JAR) --output $(ANDROID_APP_DEX_DIR) $$(find $(ANDROID_APP_CLASSES_DIR) -name '*.class' -print)

$(ANDROID_APP_KEYSTORE):
	mkdir -p $(ANDROID_APP_BUILD_DIR)
	$(KEYTOOL) -genkeypair -noprompt -keystore $@ -storetype PKCS12 -storepass android -keypass android -alias androiddebugkey -dname "CN=Android Debug,O=Android,C=US" -keyalg RSA -validity 10000

$(ANDROID_X86_64_TEST_APK): $(ANDROID_X86_64_TEST_LIB) $(ANDROID_APP_DEX) $(ANDROID_APP_KEYSTORE) $(ANDROID_APP_MANIFEST) Makefile
	rm -rf $(ANDROID_X86_64_APP_STAGING_DIR)
	mkdir -p $(ANDROID_X86_64_APP_STAGING_DIR)/lib/x86_64
	$(ANDROID_AAPT2) link -I $(ANDROID_PLATFORM_JAR) --manifest $(ANDROID_APP_MANIFEST) -o $(ANDROID_X86_64_APP_STAGING_DIR)/base.apk
	cp $(ANDROID_APP_DEX) $(ANDROID_X86_64_APP_STAGING_DIR)/classes.dex
	cp $(ANDROID_X86_64_TEST_LIB) $(ANDROID_X86_64_APP_STAGING_DIR)/lib/x86_64/libtra_ffic_android_test.so
	cd $(ANDROID_X86_64_APP_STAGING_DIR) && $(ZIP) -q -0 base.apk classes.dex lib/x86_64/libtra_ffic_android_test.so
	$(ANDROID_ZIPALIGN) -P 16 -f 4 $(ANDROID_X86_64_APP_STAGING_DIR)/base.apk $(ANDROID_X86_64_APP_STAGING_DIR)/aligned.apk
	$(ANDROID_APKSIGNER) sign --ks $(ANDROID_APP_KEYSTORE) --ks-pass pass:android --key-pass pass:android --out $@ $(ANDROID_X86_64_APP_STAGING_DIR)/aligned.apk
	$(ANDROID_APKSIGNER) verify $@

$(ANDROID_ARM64_TEST_APK): $(ANDROID_ARM64_TEST_LIB) $(ANDROID_APP_DEX) $(ANDROID_APP_KEYSTORE) $(ANDROID_APP_MANIFEST) Makefile
	rm -rf $(ANDROID_ARM64_APP_STAGING_DIR)
	mkdir -p $(ANDROID_ARM64_APP_STAGING_DIR)/lib/arm64-v8a
	$(ANDROID_AAPT2) link -I $(ANDROID_PLATFORM_JAR) --manifest $(ANDROID_APP_MANIFEST) -o $(ANDROID_ARM64_APP_STAGING_DIR)/base.apk
	cp $(ANDROID_APP_DEX) $(ANDROID_ARM64_APP_STAGING_DIR)/classes.dex
	cp $(ANDROID_ARM64_TEST_LIB) $(ANDROID_ARM64_APP_STAGING_DIR)/lib/arm64-v8a/libtra_ffic_android_test.so
	cd $(ANDROID_ARM64_APP_STAGING_DIR) && $(ZIP) -q -0 base.apk classes.dex lib/arm64-v8a/libtra_ffic_android_test.so
	$(ANDROID_ZIPALIGN) -P 16 -f 4 $(ANDROID_ARM64_APP_STAGING_DIR)/base.apk $(ANDROID_ARM64_APP_STAGING_DIR)/aligned.apk
	$(ANDROID_APKSIGNER) sign --ks $(ANDROID_APP_KEYSTORE) --ks-pass pass:android --key-pass pass:android --out $@ $(ANDROID_ARM64_APP_STAGING_DIR)/aligned.apk
	$(ANDROID_APKSIGNER) verify $@

$(WIN32_LIBFFI_STAMP):
	test -f "$(LIBFFI_SOURCE_DIR)/configure.ac" || { \
		echo "libffi submodule is not initialized; run: git submodule update --init deps/libffi" >&2; \
		exit 1; \
	}
	command -v $(WIN32_CC) >/dev/null
	command -v autoreconf >/dev/null
	command -v make >/dev/null
	rm -rf "$(WIN32_LIBFFI_BUILD_DIR)" "$(WIN32_LIBFFI_PREFIX)"
	mkdir -p "$(WIN32_LIBFFI_BUILD_DIR)"
	cp -R "$(LIBFFI_SOURCE_DIR)/." "$(WIN32_LIBFFI_BUILD_DIR)"
	rm -f "$(WIN32_LIBFFI_BUILD_DIR)/.git"
	cd "$(WIN32_LIBFFI_BUILD_DIR)" && ./autogen.sh
	cd "$(WIN32_LIBFFI_BUILD_DIR)" && ./configure --host="$(WIN32_HOST)" --prefix="$(WIN32_LIBFFI_PREFIX_ABS)" --disable-shared --enable-static --disable-docs
	$(MAKE) -C "$(WIN32_LIBFFI_BUILD_DIR)" -j"$$(nproc)"
	$(MAKE) -C "$(WIN32_LIBFFI_BUILD_DIR)" install
	touch "$(WIN32_LIBFFI_STAMP)"

$(ANDROID_X86_64_LIBFFI_STAMP): Makefile
	test -f "$(LIBFFI_SOURCE_DIR)/configure.ac" || { \
		echo "libffi submodule is not initialized; run: git submodule update --init deps/libffi" >&2; \
		exit 1; \
	}
	command -v $(ANDROID_X86_64_CC) >/dev/null
	command -v autoreconf >/dev/null
	command -v make >/dev/null
	rm -rf "$(ANDROID_X86_64_LIBFFI_BUILD_DIR)" "$(ANDROID_X86_64_LIBFFI_PREFIX)"
	mkdir -p "$(ANDROID_X86_64_LIBFFI_BUILD_DIR)"
	cp -R "$(LIBFFI_SOURCE_DIR)/." "$(ANDROID_X86_64_LIBFFI_BUILD_DIR)"
	rm -f "$(ANDROID_X86_64_LIBFFI_BUILD_DIR)/.git"
	cd "$(ANDROID_X86_64_LIBFFI_BUILD_DIR)" && ./autogen.sh
	cd "$(ANDROID_X86_64_LIBFFI_BUILD_DIR)" && \
		CC="$(ANDROID_X86_64_CC)" \
		CXX="$(ANDROID_X86_64_CXX)" \
		AR="$(ANDROID_AR)" \
		RANLIB="$(ANDROID_RANLIB)" \
		STRIP="$(ANDROID_STRIP)" \
		CFLAGS="$(ANDROID_LIBFFI_CFLAGS)" \
		LDFLAGS="$(ANDROID_LIBFFI_LDFLAGS)" \
		./configure --host="$(ANDROID_X86_64_HOST)" --prefix="$(ANDROID_X86_64_LIBFFI_PREFIX_ABS)" --disable-shared --enable-static --disable-docs --with-pic
	$(MAKE) -C "$(ANDROID_X86_64_LIBFFI_BUILD_DIR)" -j"$$(nproc)"
	$(MAKE) -C "$(ANDROID_X86_64_LIBFFI_BUILD_DIR)" install
	touch "$(ANDROID_X86_64_LIBFFI_STAMP)"

$(ANDROID_ARM64_LIBFFI_STAMP): Makefile
	test -f "$(LIBFFI_SOURCE_DIR)/configure.ac" || { \
		echo "libffi submodule is not initialized; run: git submodule update --init deps/libffi" >&2; \
		exit 1; \
	}
	command -v $(ANDROID_ARM64_CC) >/dev/null
	command -v autoreconf >/dev/null
	command -v make >/dev/null
	rm -rf "$(ANDROID_ARM64_LIBFFI_BUILD_DIR)" "$(ANDROID_ARM64_LIBFFI_PREFIX)"
	mkdir -p "$(ANDROID_ARM64_LIBFFI_BUILD_DIR)"
	cp -R "$(LIBFFI_SOURCE_DIR)/." "$(ANDROID_ARM64_LIBFFI_BUILD_DIR)"
	rm -f "$(ANDROID_ARM64_LIBFFI_BUILD_DIR)/.git"
	cd "$(ANDROID_ARM64_LIBFFI_BUILD_DIR)" && ./autogen.sh
	cd "$(ANDROID_ARM64_LIBFFI_BUILD_DIR)" && \
		CC="$(ANDROID_ARM64_CC)" \
		CXX="$(ANDROID_ARM64_CXX)" \
		AR="$(ANDROID_AR)" \
		RANLIB="$(ANDROID_RANLIB)" \
		STRIP="$(ANDROID_STRIP)" \
		CFLAGS="$(ANDROID_LIBFFI_CFLAGS)" \
		LDFLAGS="$(ANDROID_LIBFFI_LDFLAGS)" \
		./configure --host="$(ANDROID_ARM64_HOST)" --prefix="$(ANDROID_ARM64_LIBFFI_PREFIX_ABS)" --disable-shared --enable-static --disable-docs --with-pic
	$(MAKE) -C "$(ANDROID_ARM64_LIBFFI_BUILD_DIR)" -j"$$(nproc)"
	$(MAKE) -C "$(ANDROID_ARM64_LIBFFI_BUILD_DIR)" install
	touch "$(ANDROID_ARM64_LIBFFI_STAMP)"

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(ANDROID_X86_64_BUILD_DIR):
	mkdir -p $(ANDROID_X86_64_BUILD_DIR)

$(ANDROID_ARM64_BUILD_DIR):
	mkdir -p $(ANDROID_ARM64_BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
