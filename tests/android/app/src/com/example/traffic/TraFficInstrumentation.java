package com.example.traffic;

import android.app.Activity;
import android.app.Instrumentation;
import android.os.Build;
import android.os.Bundle;
import java.io.PrintWriter;
import java.io.StringWriter;

public final class TraFficInstrumentation extends Instrumentation {
  private Bundle testArguments;

  static {
    System.loadLibrary("tra_ffic_android_test");
  }

  private static native int nativeRunTests();

  private static native long nativeGetPageSize();

  @Override
  public void onCreate(Bundle arguments) {
    super.onCreate(arguments);
    testArguments = arguments;
    start();
  }

  @Override
  public void onStart() {
    final Bundle results = new Bundle();
    int resultCode = Activity.RESULT_CANCELED;

    try {
      final int expectedApi = Integer.parseInt(
          requiredArgument("expectedApi"));
      final long expectedPageSize = Long.parseLong(
          requiredArgument("expectedPageSize"));
      final String expectedAbi = requiredArgument("expectedAbi");
      if (Build.VERSION.SDK_INT != expectedApi) {
        throw new AssertionError(
            "Android API mismatch: expected " + expectedApi
                + ", got " + Build.VERSION.SDK_INT);
      }
      if (Build.SUPPORTED_ABIS.length == 0
          || !expectedAbi.equals(Build.SUPPORTED_ABIS[0])) {
        throw new AssertionError(
            "Android ABI mismatch: expected " + expectedAbi
                + ", got "
                + (Build.SUPPORTED_ABIS.length == 0
                    ? "none" : Build.SUPPORTED_ABIS[0]));
      }
      final long pageSize = nativeGetPageSize();
      if (pageSize != expectedPageSize) {
        throw new AssertionError(
            "Android page size mismatch: expected " + expectedPageSize
                + ", got " + pageSize);
      }

      final int status = nativeRunTests();
      if (status != 0) {
        throw new AssertionError(
            "native regression tests returned " + status);
      }
      results.putString(
          REPORT_KEY_STREAMRESULT, "tra_ffic_android_test: PASS\n");
      resultCode = Activity.RESULT_OK;
    } catch (Throwable error) {
      final StringWriter trace = new StringWriter();
      error.printStackTrace(new PrintWriter(trace));
      results.putString(
          REPORT_KEY_STREAMRESULT,
          "tra_ffic_android_test: FAIL\n" + trace.toString());
    }

    finish(resultCode, results);
  }

  private String requiredArgument(String name) {
    if (testArguments == null) {
      throw new IllegalArgumentException(
          "instrumentation arguments are missing");
    }
    final String value = testArguments.getString(name);
    if (value == null || value.length() == 0) {
      throw new IllegalArgumentException(
          "instrumentation argument is missing: " + name);
    }
    return value;
  }
}
