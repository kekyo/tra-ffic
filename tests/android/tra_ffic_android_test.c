/* tra-ffic - Universal asynchronous full-duplex marshaling helper library.
 * Copyright (c) Kouji Matsui (@kekyo@mi.kekyo.net)
 * Under MIT.
 * https://github.com/kekyo/tra-ffic/
 */

#include <jni.h>
#include <unistd.h>

extern int tra_ffic_android_regression_main(void);

JNIEXPORT jint JNICALL
Java_com_example_traffic_TraFficInstrumentation_nativeRunTests(
    JNIEnv *environment,
    jclass instrumentation_class) {
  (void)environment;
  (void)instrumentation_class;
  return (jint)tra_ffic_android_regression_main();
}

JNIEXPORT jlong JNICALL
Java_com_example_traffic_TraFficInstrumentation_nativeGetPageSize(
    JNIEnv *environment,
    jclass instrumentation_class) {
  (void)environment;
  (void)instrumentation_class;
  return (jlong)sysconf(_SC_PAGESIZE);
}
