/* tra-ffic - Universal asynchronous full-duplex marshaling helper library.
 * Copyright (c) Kouji Matsui (@kekyo@mi.kekyo.net)
 * Under MIT.
 * https://github.com/kekyo/tra-ffic/
 */

#pragma once

#ifndef TRA_FFIC_H
#define TRA_FFIC_H

#include <stdlib.h>

#ifndef TRA_FFIC_IN_WIN32
#if defined(_WIN32)
#define TRA_FFIC_IN_WIN32 1
#else
#define TRA_FFIC_IN_WIN32 0
#endif
#endif

#ifndef TRA_FFIC_IN_POSIX
#if TRA_FFIC_IN_WIN32
#define TRA_FFIC_IN_POSIX 0
#else
#define TRA_FFIC_IN_POSIX 1
#endif
#endif

#if TRA_FFIC_IN_POSIX && TRA_FFIC_IN_WIN32
#error "TRA_FFIC_IN_POSIX and TRA_FFIC_IN_WIN32 cannot both be enabled"
#endif

#if !TRA_FFIC_IN_POSIX && !TRA_FFIC_IN_WIN32
#error "Either TRA_FFIC_IN_POSIX or TRA_FFIC_IN_WIN32 must be enabled"
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if TRA_FFIC_IN_POSIX
#include <pthread.h>
#elif TRA_FFIC_IN_WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <ffi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef char tra_ffic_bool_must_be_one_byte[(sizeof(bool) == 1) ? 1 : -1];

/** Maximum number of bytes stored in a tra_ffic_error message. */
#define TRA_FFIC_ERROR_MESSAGE_CAPACITY 256u

/** Maximum supported recursive function type nesting depth. */
#define TRA_FFIC_MAX_TYPE_DEPTH 16u

/** Maximum supported ABI argument count, excluding the completion callback. */
#define TRA_FFIC_MAX_ARG_COUNT 64u

typedef struct tra_ffic_side tra_ffic_side;
typedef struct tra_ffic_type tra_ffic_type;
typedef struct tra_ffic_signature tra_ffic_signature;
typedef struct tra_ffic_value tra_ffic_value;
typedef struct tra_ffic_result tra_ffic_result;
typedef struct tra_ffic_function_ref tra_ffic_function_ref;

/** Error buffer filled by API calls that can fail. */
typedef struct tra_ffic_error {
  /** UTF-8 diagnostic text. Empty when no error has been written. */
  char message[TRA_FFIC_ERROR_MESSAGE_CAPACITY];
} tra_ffic_error;

/** Value kinds supported by the tra_ffic ABI. */
typedef enum tra_ffic_type_kind {
  TRA_FFIC_TYPE_VOID = 0,
  TRA_FFIC_TYPE_BOOL = 1,
  TRA_FFIC_TYPE_INT8 = 2,
  TRA_FFIC_TYPE_UINT8 = 3,
  TRA_FFIC_TYPE_INT16 = 4,
  TRA_FFIC_TYPE_UINT16 = 5,
  TRA_FFIC_TYPE_INT32 = 6,
  TRA_FFIC_TYPE_UINT32 = 7,
  TRA_FFIC_TYPE_INT64 = 8,
  TRA_FFIC_TYPE_UINT64 = 9,
  TRA_FFIC_TYPE_FLOAT = 10,
  TRA_FFIC_TYPE_DOUBLE = 11,
  TRA_FFIC_TYPE_STRING = 12,
  TRA_FFIC_TYPE_POINTER = 13,
  TRA_FFIC_TYPE_FUNCTION = 14,
  TRA_FFIC_TYPE_BUFFER_VIEW = 15,
} tra_ffic_type_kind;

/** Native function ABI used by a tra_ffic_signature. */
typedef enum tra_ffic_signature_abi {
  /** Native function has ABI void(tra_ffic_completion, ...typed_args). */
  TRA_FFIC_SIGNATURE_ABI_COMPLETION = 0,
  /** Native function has ABI return_type(...typed_args). */
  TRA_FFIC_SIGNATURE_ABI_RETVAL = 1,
} tra_ffic_signature_abi;

/** How logical arguments are exposed in the native function ABI. */
typedef enum tra_ffic_argument_passing {
  /** Arguments are passed as individual typed native parameters. */
  TRA_FFIC_ARGUMENT_PASSING_STACK = 0,
  /** Arguments are passed as one void *const * list of value addresses. */
  TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST = 1,
} tra_ffic_argument_passing;

/** Recursive type descriptor used by function signatures. */
struct tra_ffic_type {
  /** Leaf kind. Function types use function_signature. */
  tra_ffic_type_kind kind;
  /** Nested signature. Valid only when kind is TRA_FFIC_TYPE_FUNCTION. */
  const tra_ffic_signature *function_signature;
};

/** Function signature with an explicit native ABI. */
struct tra_ffic_signature {
  /** Native ABI used by this signature. */
  tra_ffic_signature_abi abi;
  /** Number of declared logical arguments. */
  uint32_t arg_count;
  /** Argument type table. Void is not valid as an argument. */
  const tra_ffic_type *arg_types;
  /** Logical return type. */
  const tra_ffic_type *return_type;
  /** Native argument layout visible to callers. Defaults to stack when zero. */
  tra_ffic_argument_passing argument_passing;
};

/** Native executable pointer created by libffi. */
typedef void (*tra_ffic_native_function)(void);

/** Internal reference to a registered function pointer. */
struct tra_ffic_function_ref {
  /** Executable native function pointer. */
  tra_ffic_native_function raw;
  /** Registry that owns raw. */
  tra_ffic_side *owner_side;
  /** Function signature associated with raw. */
  const tra_ffic_signature *signature;
};

/** Borrowed mutable byte buffer view. */
typedef struct tra_ffic_buffer_view {
  /** Borrowed pointer to the first byte. May be null only when size is zero. */
  void *data;
  /** Number of addressable bytes in data. */
  uintptr_t size;
} tra_ffic_buffer_view;

/** Runtime value used for call arguments and structured results. */
struct tra_ffic_value {
  /** Active union member. */
  tra_ffic_type_kind kind;
  union {
    /** Boolean value. */
    bool bool_value;
    /** Signed 8-bit integer value. */
    int8_t int8_value;
    /** Unsigned 8-bit integer value. */
    uint8_t uint8_value;
    /** Signed 16-bit integer value. */
    int16_t int16_value;
    /** Unsigned 16-bit integer value. */
    uint16_t uint16_value;
    /** Signed 32-bit integer value. */
    int32_t int32_value;
    /** Unsigned 32-bit integer value. */
    uint32_t uint32_value;
    /** Signed 64-bit integer value. */
    int64_t int64_value;
    /** Unsigned 64-bit integer value. */
    uint64_t uint64_value;
    /** Single precision floating-point value. */
    float float_value;
    /** Double precision floating-point value. */
    double double_value;
    /** Borrowed raw pointer. */
    void *pointer_value;
    /** Borrowed mutable byte buffer view. */
    tra_ffic_buffer_view buffer_view_value;
    /** Borrowed UTF-8 string pointer. */
    const char *string_value;
    /** Borrowed registered function pointer. May be null as a value. */
    tra_ffic_native_function function_value;
  } as;
};

/** Legacy structured result used by internal call helpers. */
struct tra_ffic_result {
  /** True when the native function completed successfully. */
  bool success;
  /** Successful value. Valid only when success is true. */
  tra_ffic_value value;
  /** Failure diagnostic. Valid only when success is false. */
  char error_message[TRA_FFIC_ERROR_MESSAGE_CAPACITY];
};

/**
 * Completion callback passed as the first native function argument.
 *
 * result points to the typed value described by the function return type.
 * When error_message is non-null, result is ignored. For a void success,
 * result may be null.
 */
typedef void (*tra_ffic_completion)(const void *result,
                                       const char *error_message);

/** Completion function that intentionally ignores success and failure. */
static inline void tra_ffic_ignored_completion(
    const void *result,
    const char *error_message) {
  (void)result;
  (void)error_message;
}

/**
 * User function pointer with a signature described by tra_ffic_signature.
 *
 * Completion ABI pure functions receive tra_ffic_completion followed by typed
 * arguments. Retval ABI pure functions return the signature return type
 * directly. Closure callbacks receive void *closure_state after completion for
 * completion ABI, or before typed arguments for retval ABI.
 */
typedef void (*tra_ffic_user_function)(void);

/**
 * User callback invoked by a completion function.
 *
 * For non-void results the callback receives void *user_data, one typed
 * result argument, then const tra_ffic_error *error. For void results it
 * receives void *user_data then const tra_ffic_error *error.
 */
typedef void (*tra_ffic_completion_callback)(void);

/**
 * Legacy callback invoked by internal call helpers for successful results.
 *
 * For non-void results the callback receives void *user_data and one typed
 * result argument. For void results it receives only void *user_data.
 */
typedef void (*tra_ffic_success_callback)(void);

/** Legacy callback invoked by internal call helpers for final results. */
typedef void (*tra_ffic_result_callback)(void *user_data,
                                            const tra_ffic_result *result);

/**
 * Callback invoked by a raw closure function.
 *
 * Raw closures still expose the ABI described by tra_ffic_signature, but the
 * callback receives the decoded argument list instead of typed C parameters.
 */
typedef void (*tra_ffic_raw_closure_callback)(
    tra_ffic_completion completion,
    void *closure_state,
    const tra_ffic_value *args,
    uint32_t arg_count);

/** Optional finalizer for closure user data. */
typedef void (*tra_ffic_finalize_user_data)(void *user_data);

/** Deferred task function used by the task queue and side scheduler. */
typedef void (*tra_ffic_task_function)(void *task_data);

/**
 * Scheduler used by tra_ffic_side.
 *
 * The scheduler must not run the task inline when called from a libffi
 * trampoline, because executable closure memory may be released by the task.
 */
typedef int (*tra_ffic_schedule_function)(void *schedule_data,
                                             tra_ffic_task_function task,
                                             void *task_data);

typedef struct tra_ffic_task_node tra_ffic_task_node;
typedef struct tra_ffic_task_queue tra_ffic_task_queue;

typedef struct tra_ffic_mutex {
#if TRA_FFIC_IN_POSIX
  pthread_mutex_t handle;
#elif TRA_FFIC_IN_WIN32
  HANDLE handle;
#endif
} tra_ffic_mutex;

#if TRA_FFIC_IN_POSIX
#define TRA_FFIC_MUTEX_STATIC_INIT \
  { PTHREAD_MUTEX_INITIALIZER }

typedef tra_ffic_mutex tra_ffic_static_mutex;
#elif TRA_FFIC_IN_WIN32
typedef struct tra_ffic_static_mutex {
  INIT_ONCE once;
  HANDLE handle;
} tra_ffic_static_mutex;

#define TRA_FFIC_MUTEX_STATIC_INIT \
  { INIT_ONCE_STATIC_INIT, NULL }
#endif

/**
 * Callback invoked when queued finalization work should be drained.
 *
 * @param queue The task queue that received new work.
 * @param state User-provided callback state.
 */
typedef void (*tra_ffic_task_drain_finalization_callback)(
    tra_ffic_task_queue *queue,
    void *state);

/** Thread-safe FIFO queue for deferred tra_ffic tasks. */
struct tra_ffic_task_queue {
  /** Internal mutex. */
  tra_ffic_mutex mutex;
  /** First queued task. */
  tra_ffic_task_node *head;
  /** Last queued task. */
  tra_ffic_task_node *tail;
  /** Optional callback invoked after successful task enqueue. */
  tra_ffic_task_drain_finalization_callback drain_finalization_callback;
  /** User state passed to drain_finalization_callback. */
  void *drain_finalization_state;
  /** Whether mutex has been initialized. */
  bool initialized;
};

typedef struct tra_ffic_registry_entry tra_ffic_registry_entry;
typedef struct tra_ffic_function_adapter_state
    tra_ffic_function_adapter_state;

/** One side of a same-process A/B function marshaling pair. */
struct tra_ffic_side {
  /** Internal registry mutex. */
  tra_ffic_mutex mutex;
  /** Live registry entries owned by this side. */
  tra_ffic_registry_entry *entries;
  /** Opposite side in the pair. */
  tra_ffic_side *peer;
  /** Deferred task scheduler. */
  tra_ffic_schedule_function schedule;
  /** Opaque scheduler state. */
  void *schedule_data;
  /** Whether this side was initialized. */
  bool initialized;
};

/** Snapshot of libffi closure allocation counters. */
typedef struct tra_ffic_closure_tracker_snapshot {
  /** Number of successful ffi_closure_alloc calls. */
  uint64_t alloc_count;
  /** Number of ffi_closure_free calls. */
  uint64_t free_count;
  /** Number of currently live closures. */
  uint64_t live_count;
  /** Highest observed live closure count. */
  uint64_t high_water;
} tra_ffic_closure_tracker_snapshot;

struct tra_ffic_task_node {
  tra_ffic_task_function task;
  void *task_data;
  tra_ffic_task_node *next;
};

struct tra_ffic_registry_entry {
  tra_ffic_side *owner_side;
  tra_ffic_registry_entry *next;
  tra_ffic_registry_entry *global_next;
  tra_ffic_signature *signature;
  tra_ffic_type completion_return_type;
  bool has_completion_return_type;
  ffi_cif cif;
  ffi_type **ffi_arg_types;
  uint32_t ffi_arg_count;
  ffi_cif callback_cif;
  ffi_type **callback_arg_types;
  uint32_t callback_arg_count;
  ffi_closure *closure;
  void *closure_code;
  tra_ffic_native_function raw;
  tra_ffic_user_function callback;
  tra_ffic_raw_closure_callback raw_closure_callback;
  void *user_data;
  tra_ffic_finalize_user_data finalize_user_data;
  tra_ffic_argument_passing callback_argument_passing;
  uint32_t ref_count;
  uint32_t active_call_count;
  bool destroy_scheduled;
  bool passes_closure_state;
  bool is_completion_function;
  bool completion_claimed;
};

typedef union tra_ffic_code_to_native {
  void *object_pointer;
  tra_ffic_native_function native_function;
} tra_ffic_code_to_native;

typedef union tra_ffic_code_to_completion {
  void *object_pointer;
  tra_ffic_completion completion;
} tra_ffic_code_to_completion;

static tra_ffic_static_mutex tra_ffic_global_registry_mutex =
    TRA_FFIC_MUTEX_STATIC_INIT;
static tra_ffic_registry_entry *tra_ffic_global_registry_entries = NULL;

#if UINTPTR_MAX == UINT32_MAX
#define TRA_FFIC_FFI_TYPE_UINTPTR (&ffi_type_uint32)
#elif UINTPTR_MAX == UINT64_MAX
#define TRA_FFIC_FFI_TYPE_UINTPTR (&ffi_type_uint64)
#else
#error "Unsupported uintptr_t size"
#endif

static ffi_type *tra_ffic_buffer_view_ffi_elements[] = {
    &ffi_type_pointer,
    TRA_FFIC_FFI_TYPE_UINTPTR,
    NULL,
};
static ffi_type tra_ffic_buffer_view_ffi_type = {
    0u,
    0u,
    FFI_TYPE_STRUCT,
    tra_ffic_buffer_view_ffi_elements,
};

#if defined(TRA_FFIC_TRACK_CLOSURES)
static tra_ffic_static_mutex tra_ffic_tracker_mutex =
    TRA_FFIC_MUTEX_STATIC_INIT;
static uint64_t tra_ffic_tracker_alloc_count = 0;
static uint64_t tra_ffic_tracker_free_count = 0;
static uint64_t tra_ffic_tracker_live_count = 0;
static uint64_t tra_ffic_tracker_high_water = 0;
#endif

static inline int tra_ffic_mutex_init(tra_ffic_mutex *mutex) {
  if (mutex == NULL) {
    return 0;
  }
#if TRA_FFIC_IN_POSIX
  return pthread_mutex_init(&mutex->handle, NULL) == 0;
#elif TRA_FFIC_IN_WIN32
  mutex->handle = CreateMutexA(NULL, FALSE, NULL);
  return mutex->handle != NULL;
#endif
}

static inline void tra_ffic_mutex_lock(tra_ffic_mutex *mutex) {
  if (mutex == NULL) {
    return;
  }
#if TRA_FFIC_IN_POSIX
  pthread_mutex_lock(&mutex->handle);
#elif TRA_FFIC_IN_WIN32
  if (mutex->handle != NULL) {
    (void)WaitForSingleObject(mutex->handle, INFINITE);
  }
#endif
}

static inline void tra_ffic_mutex_unlock(tra_ffic_mutex *mutex) {
  if (mutex == NULL) {
    return;
  }
#if TRA_FFIC_IN_POSIX
  pthread_mutex_unlock(&mutex->handle);
#elif TRA_FFIC_IN_WIN32
  if (mutex->handle != NULL) {
    (void)ReleaseMutex(mutex->handle);
  }
#endif
}

static inline void tra_ffic_mutex_destroy(tra_ffic_mutex *mutex) {
  if (mutex == NULL) {
    return;
  }
#if TRA_FFIC_IN_POSIX
  pthread_mutex_destroy(&mutex->handle);
#elif TRA_FFIC_IN_WIN32
  if (mutex->handle != NULL) {
    CloseHandle(mutex->handle);
    mutex->handle = NULL;
  }
#endif
}

#if TRA_FFIC_IN_WIN32
static BOOL CALLBACK tra_ffic_static_mutex_init_once(
    PINIT_ONCE init_once,
    PVOID parameter,
    PVOID *context) {
  tra_ffic_static_mutex *mutex = (tra_ffic_static_mutex *)parameter;
  (void)init_once;
  (void)context;
  if (mutex == NULL) {
    return FALSE;
  }
  mutex->handle = CreateMutexA(NULL, FALSE, NULL);
  return mutex->handle != NULL ? TRUE : FALSE;
}
#endif

static inline int tra_ffic_static_mutex_ensure(
    tra_ffic_static_mutex *mutex) {
  if (mutex == NULL) {
    return 0;
  }
#if TRA_FFIC_IN_POSIX
  return 1;
#elif TRA_FFIC_IN_WIN32
  return InitOnceExecuteOnce(&mutex->once, tra_ffic_static_mutex_init_once,
                             mutex, NULL) != 0 &&
         mutex->handle != NULL;
#endif
}

static inline void tra_ffic_static_mutex_lock(
    tra_ffic_static_mutex *mutex) {
#if TRA_FFIC_IN_POSIX
  tra_ffic_mutex_lock(mutex);
#elif TRA_FFIC_IN_WIN32
  if (tra_ffic_static_mutex_ensure(mutex)) {
    (void)WaitForSingleObject(mutex->handle, INFINITE);
  }
#endif
}

static inline void tra_ffic_static_mutex_unlock(
    tra_ffic_static_mutex *mutex) {
#if TRA_FFIC_IN_POSIX
  tra_ffic_mutex_unlock(mutex);
#elif TRA_FFIC_IN_WIN32
  if (mutex != NULL && mutex->handle != NULL) {
    (void)ReleaseMutex(mutex->handle);
  }
#endif
}

/** Clears a tra_ffic_error buffer. */
static inline void tra_ffic_error_clear(tra_ffic_error *error) {
  if (error != NULL) {
    error->message[0] = '\0';
  }
}

/** Writes a diagnostic into a tra_ffic_error buffer. */
static inline void tra_ffic_error_set(tra_ffic_error *error,
                                         const char *message) {
  if (error == NULL) {
    return;
  }
  if (message == NULL) {
    message = "tra_ffic error";
  }
  (void)snprintf(error->message, sizeof(error->message), "%s", message);
}

static inline char *tra_ffic_string_duplicate(const char *value) {
  size_t length = 0;
  char *copy = NULL;
  if (value == NULL) {
    return NULL;
  }
  length = strlen(value);
  copy = (char *)malloc(length + 1u);
  if (copy == NULL) {
    return NULL;
  }
  memcpy(copy, value, length + 1u);
  return copy;
}

/** Returns a void type descriptor for function results. */
static inline tra_ffic_type tra_ffic_type_void(void) {
  tra_ffic_type type;
  type.kind = TRA_FFIC_TYPE_VOID;
  type.function_signature = NULL;
  return type;
}

/** Returns a bool type descriptor. */
static inline tra_ffic_type tra_ffic_type_bool(void) {
  tra_ffic_type type;
  type.kind = TRA_FFIC_TYPE_BOOL;
  type.function_signature = NULL;
  return type;
}

/** Returns an int8_t type descriptor. */
static inline tra_ffic_type tra_ffic_type_int8(void) {
  tra_ffic_type type;
  type.kind = TRA_FFIC_TYPE_INT8;
  type.function_signature = NULL;
  return type;
}

/** Returns a uint8_t type descriptor. */
static inline tra_ffic_type tra_ffic_type_uint8(void) {
  tra_ffic_type type;
  type.kind = TRA_FFIC_TYPE_UINT8;
  type.function_signature = NULL;
  return type;
}

/** Returns an int16_t type descriptor. */
static inline tra_ffic_type tra_ffic_type_int16(void) {
  tra_ffic_type type;
  type.kind = TRA_FFIC_TYPE_INT16;
  type.function_signature = NULL;
  return type;
}

/** Returns a uint16_t type descriptor. */
static inline tra_ffic_type tra_ffic_type_uint16(void) {
  tra_ffic_type type;
  type.kind = TRA_FFIC_TYPE_UINT16;
  type.function_signature = NULL;
  return type;
}

/** Returns an int32_t type descriptor. */
static inline tra_ffic_type tra_ffic_type_int32(void) {
  tra_ffic_type type;
  type.kind = TRA_FFIC_TYPE_INT32;
  type.function_signature = NULL;
  return type;
}

/** Returns a uint32_t type descriptor. */
static inline tra_ffic_type tra_ffic_type_uint32(void) {
  tra_ffic_type type;
  type.kind = TRA_FFIC_TYPE_UINT32;
  type.function_signature = NULL;
  return type;
}

/** Returns an int64_t type descriptor. */
static inline tra_ffic_type tra_ffic_type_int64(void) {
  tra_ffic_type type;
  type.kind = TRA_FFIC_TYPE_INT64;
  type.function_signature = NULL;
  return type;
}

/** Returns a uint64_t type descriptor. */
static inline tra_ffic_type tra_ffic_type_uint64(void) {
  tra_ffic_type type;
  type.kind = TRA_FFIC_TYPE_UINT64;
  type.function_signature = NULL;
  return type;
}

/** Returns a float type descriptor. */
static inline tra_ffic_type tra_ffic_type_float(void) {
  tra_ffic_type type;
  type.kind = TRA_FFIC_TYPE_FLOAT;
  type.function_signature = NULL;
  return type;
}

/** Returns a double type descriptor. */
static inline tra_ffic_type tra_ffic_type_double(void) {
  tra_ffic_type type;
  type.kind = TRA_FFIC_TYPE_DOUBLE;
  type.function_signature = NULL;
  return type;
}

/** Returns a const char* string type descriptor. */
static inline tra_ffic_type tra_ffic_type_string(void) {
  tra_ffic_type type;
  type.kind = TRA_FFIC_TYPE_STRING;
  type.function_signature = NULL;
  return type;
}

/** Returns a borrowed raw pointer type descriptor. */
static inline tra_ffic_type tra_ffic_type_pointer(void) {
  tra_ffic_type type;
  type.kind = TRA_FFIC_TYPE_POINTER;
  type.function_signature = NULL;
  return type;
}

/** Returns a borrowed mutable byte buffer view type descriptor. */
static inline tra_ffic_type tra_ffic_type_buffer_view(void) {
  tra_ffic_type type;
  type.kind = TRA_FFIC_TYPE_BUFFER_VIEW;
  type.function_signature = NULL;
  return type;
}

/** Returns a closed function type descriptor for the given signature. */
static inline tra_ffic_type tra_ffic_type_function(
    const tra_ffic_signature *signature) {
  tra_ffic_type type;
  type.kind = TRA_FFIC_TYPE_FUNCTION;
  type.function_signature = signature;
  return type;
}

/** Returns a stack-argument function signature descriptor. */
static inline tra_ffic_signature tra_ffic_signature_stack(
    tra_ffic_signature_abi abi,
    uint32_t arg_count,
    const tra_ffic_type *arg_types,
    const tra_ffic_type *return_type) {
  tra_ffic_signature signature;
  signature.abi = abi;
  signature.arg_count = arg_count;
  signature.arg_types = arg_types;
  signature.return_type = return_type;
  signature.argument_passing = TRA_FFIC_ARGUMENT_PASSING_STACK;
  return signature;
}

/** Returns a pointer-list-argument function signature descriptor. */
static inline tra_ffic_signature tra_ffic_signature_pointer_list(
    tra_ffic_signature_abi abi,
    uint32_t arg_count,
    const tra_ffic_type *arg_types,
    const tra_ffic_type *return_type) {
  tra_ffic_signature signature =
      tra_ffic_signature_stack(abi, arg_count, arg_types, return_type);
  signature.argument_passing = TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST;
  return signature;
}

/** Returns a void runtime value. */
static inline tra_ffic_value tra_ffic_value_void(void) {
  tra_ffic_value value;
  value.kind = TRA_FFIC_TYPE_VOID;
  value.as.int32_value = 0;
  return value;
}

/** Returns a bool runtime value. */
static inline tra_ffic_value tra_ffic_value_bool(bool source) {
  tra_ffic_value value;
  value.kind = TRA_FFIC_TYPE_BOOL;
  value.as.bool_value = source;
  return value;
}

/** Returns an int8_t runtime value. */
static inline tra_ffic_value tra_ffic_value_int8(int8_t source) {
  tra_ffic_value value;
  value.kind = TRA_FFIC_TYPE_INT8;
  value.as.int8_value = source;
  return value;
}

/** Returns a uint8_t runtime value. */
static inline tra_ffic_value tra_ffic_value_uint8(uint8_t source) {
  tra_ffic_value value;
  value.kind = TRA_FFIC_TYPE_UINT8;
  value.as.uint8_value = source;
  return value;
}

/** Returns an int16_t runtime value. */
static inline tra_ffic_value tra_ffic_value_int16(int16_t source) {
  tra_ffic_value value;
  value.kind = TRA_FFIC_TYPE_INT16;
  value.as.int16_value = source;
  return value;
}

/** Returns a uint16_t runtime value. */
static inline tra_ffic_value tra_ffic_value_uint16(uint16_t source) {
  tra_ffic_value value;
  value.kind = TRA_FFIC_TYPE_UINT16;
  value.as.uint16_value = source;
  return value;
}

/** Returns an int32_t runtime value. */
static inline tra_ffic_value tra_ffic_value_int32(int32_t source) {
  tra_ffic_value value;
  value.kind = TRA_FFIC_TYPE_INT32;
  value.as.int32_value = source;
  return value;
}

/** Returns a uint32_t runtime value. */
static inline tra_ffic_value tra_ffic_value_uint32(uint32_t source) {
  tra_ffic_value value;
  value.kind = TRA_FFIC_TYPE_UINT32;
  value.as.uint32_value = source;
  return value;
}

/** Returns an int64_t runtime value. */
static inline tra_ffic_value tra_ffic_value_int64(int64_t source) {
  tra_ffic_value value;
  value.kind = TRA_FFIC_TYPE_INT64;
  value.as.int64_value = source;
  return value;
}

/** Returns a uint64_t runtime value. */
static inline tra_ffic_value tra_ffic_value_uint64(uint64_t source) {
  tra_ffic_value value;
  value.kind = TRA_FFIC_TYPE_UINT64;
  value.as.uint64_value = source;
  return value;
}

/** Returns a float runtime value. */
static inline tra_ffic_value tra_ffic_value_float(float source) {
  tra_ffic_value value;
  value.kind = TRA_FFIC_TYPE_FLOAT;
  value.as.float_value = source;
  return value;
}

/** Returns a double runtime value. */
static inline tra_ffic_value tra_ffic_value_double(double source) {
  tra_ffic_value value;
  value.kind = TRA_FFIC_TYPE_DOUBLE;
  value.as.double_value = source;
  return value;
}

/** Returns a borrowed const char* string runtime value. */
static inline tra_ffic_value tra_ffic_value_string(const char *source) {
  tra_ffic_value value;
  value.kind = TRA_FFIC_TYPE_STRING;
  value.as.string_value = source;
  return value;
}

/** Returns a borrowed raw pointer runtime value. */
static inline tra_ffic_value tra_ffic_value_pointer(void *source) {
  tra_ffic_value value;
  value.kind = TRA_FFIC_TYPE_POINTER;
  value.as.pointer_value = source;
  return value;
}

/** Returns a borrowed mutable byte buffer view runtime value. */
static inline tra_ffic_value tra_ffic_value_buffer_view(
    void *data,
    uintptr_t size) {
  tra_ffic_value value;
  value.kind = TRA_FFIC_TYPE_BUFFER_VIEW;
  value.as.buffer_view_value.data = data;
  value.as.buffer_view_value.size = size;
  return value;
}

/** Returns a borrowed closed function runtime value. source may be null. */
static inline tra_ffic_value tra_ffic_value_function(
    tra_ffic_native_function source) {
  tra_ffic_value value;
  value.kind = TRA_FFIC_TYPE_FUNCTION;
  value.as.function_value = source;
  return value;
}

static inline tra_ffic_native_function tra_ffic_native_from_code(
    void *code) {
  tra_ffic_code_to_native converter;
  converter.object_pointer = code;
  return converter.native_function;
}

static inline tra_ffic_completion tra_ffic_completion_from_code(
    void *code) {
  tra_ffic_code_to_completion converter;
  converter.object_pointer = code;
  return converter.completion;
}

static inline uintptr_t tra_ffic_function_key(
    tra_ffic_native_function raw) {
  return (uintptr_t)raw;
}

static inline void tra_ffic_tracker_after_alloc(void) {
#if defined(TRA_FFIC_TRACK_CLOSURES)
  tra_ffic_static_mutex_lock(&tra_ffic_tracker_mutex);
  tra_ffic_tracker_alloc_count += 1u;
  tra_ffic_tracker_live_count += 1u;
  if (tra_ffic_tracker_high_water < tra_ffic_tracker_live_count) {
    tra_ffic_tracker_high_water = tra_ffic_tracker_live_count;
  }
  tra_ffic_static_mutex_unlock(&tra_ffic_tracker_mutex);
#endif
}

static inline void tra_ffic_tracker_after_free(void) {
#if defined(TRA_FFIC_TRACK_CLOSURES)
  tra_ffic_static_mutex_lock(&tra_ffic_tracker_mutex);
  tra_ffic_tracker_free_count += 1u;
  if (tra_ffic_tracker_live_count > 0u) {
    tra_ffic_tracker_live_count -= 1u;
  }
  tra_ffic_static_mutex_unlock(&tra_ffic_tracker_mutex);
#endif
}

static inline ffi_closure *tra_ffic_closure_alloc_tracked(void **code) {
  ffi_closure *closure = (ffi_closure *)ffi_closure_alloc(sizeof(ffi_closure),
                                                          code);
  if (closure != NULL) {
    tra_ffic_tracker_after_alloc();
  }
  return closure;
}

static inline void tra_ffic_closure_free_tracked(ffi_closure *closure) {
  if (closure == NULL) {
    return;
  }
  tra_ffic_tracker_after_free();
  ffi_closure_free(closure);
}

/** Returns the current libffi closure allocation tracker snapshot. */
static inline tra_ffic_closure_tracker_snapshot
tra_ffic_get_closure_tracker_snapshot(void) {
  tra_ffic_closure_tracker_snapshot snapshot;
  snapshot.alloc_count = 0u;
  snapshot.free_count = 0u;
  snapshot.live_count = 0u;
  snapshot.high_water = 0u;
#if defined(TRA_FFIC_TRACK_CLOSURES)
  tra_ffic_static_mutex_lock(&tra_ffic_tracker_mutex);
  snapshot.alloc_count = tra_ffic_tracker_alloc_count;
  snapshot.free_count = tra_ffic_tracker_free_count;
  snapshot.live_count = tra_ffic_tracker_live_count;
  snapshot.high_water = tra_ffic_tracker_high_water;
  tra_ffic_static_mutex_unlock(&tra_ffic_tracker_mutex);
#endif
  return snapshot;
}

static inline int tra_ffic_is_valid_kind(tra_ffic_type_kind kind) {
  return kind == TRA_FFIC_TYPE_VOID || kind == TRA_FFIC_TYPE_BOOL ||
         kind == TRA_FFIC_TYPE_INT8 || kind == TRA_FFIC_TYPE_UINT8 ||
         kind == TRA_FFIC_TYPE_INT16 || kind == TRA_FFIC_TYPE_UINT16 ||
         kind == TRA_FFIC_TYPE_INT32 || kind == TRA_FFIC_TYPE_UINT32 ||
         kind == TRA_FFIC_TYPE_INT64 || kind == TRA_FFIC_TYPE_UINT64 ||
         kind == TRA_FFIC_TYPE_FLOAT || kind == TRA_FFIC_TYPE_DOUBLE ||
         kind == TRA_FFIC_TYPE_STRING ||
         kind == TRA_FFIC_TYPE_POINTER ||
         kind == TRA_FFIC_TYPE_BUFFER_VIEW ||
         kind == TRA_FFIC_TYPE_FUNCTION;
}

static inline int tra_ffic_is_valid_signature_abi(
    tra_ffic_signature_abi abi) {
  return abi == TRA_FFIC_SIGNATURE_ABI_COMPLETION ||
         abi == TRA_FFIC_SIGNATURE_ABI_RETVAL;
}

static inline int tra_ffic_is_valid_argument_passing(
    tra_ffic_argument_passing argument_passing) {
  return argument_passing == TRA_FFIC_ARGUMENT_PASSING_STACK ||
         argument_passing == TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST;
}

static inline int tra_ffic_type_validate(const tra_ffic_type *type,
                                            bool allow_void,
                                            uint32_t depth,
                                            tra_ffic_error *error);

static inline int tra_ffic_signature_validate(
    const tra_ffic_signature *signature,
    uint32_t depth,
    tra_ffic_error *error) {
  uint32_t index = 0u;
  if (signature == NULL) {
    tra_ffic_error_set(error, "Signature is null");
    return 0;
  }
  if (!tra_ffic_is_valid_signature_abi(signature->abi)) {
    tra_ffic_error_set(error, "Signature ABI is unsupported");
    return 0;
  }
  if (!tra_ffic_is_valid_argument_passing(signature->argument_passing)) {
    tra_ffic_error_set(error, "Signature argument passing is unsupported");
    return 0;
  }
  if (signature->arg_count > TRA_FFIC_MAX_ARG_COUNT) {
    tra_ffic_error_set(error, "Signature has too many arguments");
    return 0;
  }
  if (signature->arg_count > 0u && signature->arg_types == NULL) {
    tra_ffic_error_set(error, "Signature argument type table is null");
    return 0;
  }
  if (signature->return_type == NULL) {
    tra_ffic_error_set(error, "Signature return type is null");
    return 0;
  }
  for (index = 0u; index < signature->arg_count; ++index) {
    if (!tra_ffic_type_validate(&signature->arg_types[index], false,
                                   depth + 1u, error)) {
      return 0;
    }
  }
  return tra_ffic_type_validate(signature->return_type, true, depth + 1u,
                                   error);
}

static inline int tra_ffic_type_validate(const tra_ffic_type *type,
                                            bool allow_void,
                                            uint32_t depth,
                                            tra_ffic_error *error) {
  if (type == NULL) {
    tra_ffic_error_set(error, "Type is null");
    return 0;
  }
  if (depth > TRA_FFIC_MAX_TYPE_DEPTH) {
    tra_ffic_error_set(error, "Function type nesting is too deep");
    return 0;
  }
  if (!tra_ffic_is_valid_kind(type->kind)) {
    tra_ffic_error_set(error, "Type kind is unsupported");
    return 0;
  }
  if (!allow_void && type->kind == TRA_FFIC_TYPE_VOID) {
    tra_ffic_error_set(error, "Void is not valid as an argument");
    return 0;
  }
  if (type->kind == TRA_FFIC_TYPE_FUNCTION) {
    return tra_ffic_signature_validate(type->function_signature, depth + 1u,
                                          error);
  }
  if (type->function_signature != NULL) {
    tra_ffic_error_set(error, "Primitive type has a function signature");
    return 0;
  }
  return 1;
}

static inline int tra_ffic_type_equals(const tra_ffic_type *first,
                                          const tra_ffic_type *second);

static inline int tra_ffic_signature_equals(
    const tra_ffic_signature *first,
    const tra_ffic_signature *second) {
  uint32_t index = 0u;
  if (first == NULL || second == NULL) {
    return first == second;
  }
  if (first->abi != second->abi) {
    return 0;
  }
  if (first->argument_passing != second->argument_passing) {
    return 0;
  }
  if (first->arg_count != second->arg_count) {
    return 0;
  }
  for (index = 0u; index < first->arg_count; ++index) {
    if (!tra_ffic_type_equals(&first->arg_types[index],
                                 &second->arg_types[index])) {
      return 0;
    }
  }
  return tra_ffic_type_equals(first->return_type, second->return_type);
}

static inline int tra_ffic_type_equals(const tra_ffic_type *first,
                                          const tra_ffic_type *second) {
  if (first == NULL || second == NULL) {
    return first == second;
  }
  if (first->kind != second->kind) {
    return 0;
  }
  if (first->kind != TRA_FFIC_TYPE_FUNCTION) {
    return 1;
  }
  return tra_ffic_signature_equals(first->function_signature,
                                      second->function_signature);
}

static inline int tra_ffic_type_logically_equals(
    const tra_ffic_type *first,
    const tra_ffic_type *second);

static inline int tra_ffic_signature_logically_equals(
    const tra_ffic_signature *first,
    const tra_ffic_signature *second) {
  uint32_t index = 0u;
  if (first == NULL || second == NULL) {
    return first == second;
  }
  if (first->abi != second->abi || first->arg_count != second->arg_count) {
    return 0;
  }
  for (index = 0u; index < first->arg_count; ++index) {
    if (!tra_ffic_type_logically_equals(&first->arg_types[index],
                                        &second->arg_types[index])) {
      return 0;
    }
  }
  return tra_ffic_type_logically_equals(first->return_type,
                                        second->return_type);
}

static inline int tra_ffic_type_logically_equals(
    const tra_ffic_type *first,
    const tra_ffic_type *second) {
  if (first == NULL || second == NULL) {
    return first == second;
  }
  if (first->kind != second->kind) {
    return 0;
  }
  if (first->kind != TRA_FFIC_TYPE_FUNCTION) {
    return 1;
  }
  return tra_ffic_signature_logically_equals(first->function_signature,
                                            second->function_signature);
}

static inline void tra_ffic_type_destroy(tra_ffic_type *type);

static inline void tra_ffic_signature_destroy(
    tra_ffic_signature *signature) {
  uint32_t index = 0u;
  tra_ffic_type *mutable_args = NULL;
  tra_ffic_type *mutable_return = NULL;
  if (signature == NULL) {
    return;
  }
  mutable_args = (tra_ffic_type *)signature->arg_types;
  mutable_return = (tra_ffic_type *)signature->return_type;
  for (index = 0u; index < signature->arg_count; ++index) {
    tra_ffic_type_destroy(&mutable_args[index]);
  }
  tra_ffic_type_destroy(mutable_return);
  free(mutable_args);
  free(mutable_return);
  free(signature);
}

static inline void tra_ffic_type_destroy(tra_ffic_type *type) {
  if (type == NULL) {
    return;
  }
  if (type->kind == TRA_FFIC_TYPE_FUNCTION) {
    tra_ffic_signature_destroy(
        (tra_ffic_signature *)type->function_signature);
  }
  type->kind = TRA_FFIC_TYPE_VOID;
  type->function_signature = NULL;
}

static inline int tra_ffic_type_clone(const tra_ffic_type *source,
                                         tra_ffic_type *target,
                                         uint32_t depth,
                                         tra_ffic_error *error);

static inline tra_ffic_signature *tra_ffic_signature_clone(
    const tra_ffic_signature *source,
    uint32_t depth,
    tra_ffic_error *error) {
  tra_ffic_signature *target = NULL;
  tra_ffic_type *arg_types = NULL;
  tra_ffic_type *return_type = NULL;
  uint32_t index = 0u;
  if (!tra_ffic_signature_validate(source, depth, error)) {
    return NULL;
  }
  target = (tra_ffic_signature *)calloc(1u, sizeof(*target));
  return_type = (tra_ffic_type *)calloc(1u, sizeof(*return_type));
  if (source->arg_count > 0u) {
    arg_types =
        (tra_ffic_type *)calloc(source->arg_count, sizeof(*arg_types));
  }
  if (target == NULL || return_type == NULL ||
      (source->arg_count > 0u && arg_types == NULL)) {
    free(target);
    free(return_type);
    free(arg_types);
    tra_ffic_error_set(error, "Out of memory while cloning signature");
    return NULL;
  }
  target->abi = source->abi;
  target->arg_count = source->arg_count;
  target->arg_types = arg_types;
  target->return_type = return_type;
  target->argument_passing = source->argument_passing;
  for (index = 0u; index < source->arg_count; ++index) {
    if (!tra_ffic_type_clone(&source->arg_types[index], &arg_types[index],
                                depth + 1u, error)) {
      tra_ffic_signature_destroy(target);
      return NULL;
    }
  }
  if (!tra_ffic_type_clone(source->return_type, return_type, depth + 1u,
                              error)) {
    tra_ffic_signature_destroy(target);
    return NULL;
  }
  return target;
}

static inline int tra_ffic_type_clone(const tra_ffic_type *source,
                                         tra_ffic_type *target,
                                         uint32_t depth,
                                         tra_ffic_error *error) {
  if (!tra_ffic_type_validate(source, true, depth, error)) {
    return 0;
  }
  target->kind = source->kind;
  target->function_signature = NULL;
  if (source->kind == TRA_FFIC_TYPE_FUNCTION) {
    target->function_signature =
        tra_ffic_signature_clone(source->function_signature, depth + 1u,
                                    error);
    if (target->function_signature == NULL) {
      return 0;
    }
  }
  return 1;
}

static inline ffi_type *tra_ffic_get_ffi_type(
    const tra_ffic_type *type,
    bool for_return) {
  if (type == NULL) {
    return NULL;
  }
  switch (type->kind) {
    case TRA_FFIC_TYPE_VOID:
      return for_return ? &ffi_type_void : NULL;
    case TRA_FFIC_TYPE_BOOL:
      return &ffi_type_uint8;
    case TRA_FFIC_TYPE_INT8:
      return &ffi_type_sint8;
    case TRA_FFIC_TYPE_UINT8:
      return &ffi_type_uint8;
    case TRA_FFIC_TYPE_INT16:
      return &ffi_type_sint16;
    case TRA_FFIC_TYPE_UINT16:
      return &ffi_type_uint16;
    case TRA_FFIC_TYPE_INT32:
      return &ffi_type_sint32;
    case TRA_FFIC_TYPE_UINT32:
      return &ffi_type_uint32;
    case TRA_FFIC_TYPE_INT64:
      return &ffi_type_sint64;
    case TRA_FFIC_TYPE_UINT64:
      return &ffi_type_uint64;
    case TRA_FFIC_TYPE_FLOAT:
      return &ffi_type_float;
    case TRA_FFIC_TYPE_DOUBLE:
      return &ffi_type_double;
    case TRA_FFIC_TYPE_POINTER:
      return &ffi_type_pointer;
    case TRA_FFIC_TYPE_BUFFER_VIEW:
      return &tra_ffic_buffer_view_ffi_type;
    case TRA_FFIC_TYPE_STRING:
      return &ffi_type_pointer;
    case TRA_FFIC_TYPE_FUNCTION:
      return &ffi_type_pointer;
  }
  return NULL;
}

static inline ffi_type *tra_ffic_get_callback_ffi_type(
    const tra_ffic_type *type) {
  return tra_ffic_get_ffi_type(type, false);
}

static inline bool tra_ffic_signature_uses_completion(
    const tra_ffic_signature *signature) {
  return signature != NULL &&
         signature->abi == TRA_FFIC_SIGNATURE_ABI_COMPLETION;
}

static inline uint32_t tra_ffic_public_ffi_arg_count(
    const tra_ffic_signature *signature) {
  const uint32_t completion_count =
      tra_ffic_signature_uses_completion(signature) ? 1u : 0u;
  if (signature == NULL) {
    return 0u;
  }
  if (signature->argument_passing ==
      TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST) {
    return completion_count + 1u;
  }
  return completion_count + signature->arg_count;
}

static inline int tra_ffic_fill_public_ffi_arg_types(
    const tra_ffic_signature *signature,
    ffi_type **ffi_arg_types,
    tra_ffic_error *error) {
  uint32_t index = 0u;
  uint32_t offset = 0u;
  if (signature == NULL ||
      (tra_ffic_public_ffi_arg_count(signature) > 0u &&
       ffi_arg_types == NULL)) {
    tra_ffic_error_set(error, "Public ffi argument input is null");
    return 0;
  }
  if (tra_ffic_signature_uses_completion(signature)) {
    ffi_arg_types[offset] = &ffi_type_pointer;
    offset += 1u;
  }
  if (signature->argument_passing ==
      TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST) {
    ffi_arg_types[offset] = &ffi_type_pointer;
    return 1;
  }
  for (index = 0u; index < signature->arg_count; ++index) {
    ffi_arg_types[index + offset] =
        tra_ffic_get_ffi_type(&signature->arg_types[index], false);
    if (ffi_arg_types[index + offset] == NULL) {
      tra_ffic_error_set(error, "Unsupported closure argument type");
      return 0;
    }
  }
  return 1;
}

static inline uint32_t tra_ffic_callback_ffi_arg_count(
    const tra_ffic_signature *signature,
    tra_ffic_argument_passing callback_argument_passing,
    bool passes_closure_state) {
  uint32_t count = 0u;
  if (signature == NULL) {
    return 0u;
  }
  if (tra_ffic_signature_uses_completion(signature)) {
    count += 1u;
  }
  if (passes_closure_state) {
    count += 1u;
  }
  if (callback_argument_passing ==
      TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST) {
    count += 1u;
  } else {
    count += signature->arg_count;
  }
  return count;
}

static inline int tra_ffic_fill_callback_ffi_arg_types(
    const tra_ffic_signature *signature,
    tra_ffic_argument_passing callback_argument_passing,
    bool passes_closure_state,
    ffi_type **callback_arg_types,
    tra_ffic_error *error) {
  uint32_t index = 0u;
  uint32_t offset = 0u;
  if (signature == NULL ||
      (tra_ffic_callback_ffi_arg_count(
           signature, callback_argument_passing, passes_closure_state) > 0u &&
       callback_arg_types == NULL)) {
    tra_ffic_error_set(error, "Callback ffi argument input is null");
    return 0;
  }
  if (tra_ffic_signature_uses_completion(signature)) {
    callback_arg_types[offset] = &ffi_type_pointer;
    offset += 1u;
  }
  if (passes_closure_state) {
    callback_arg_types[offset] = &ffi_type_pointer;
    offset += 1u;
  }
  if (callback_argument_passing ==
      TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST) {
    callback_arg_types[offset] = &ffi_type_pointer;
    return 1;
  }
  for (index = 0u; index < signature->arg_count; ++index) {
    callback_arg_types[index + offset] =
        tra_ffic_get_callback_ffi_type(&signature->arg_types[index]);
    if (callback_arg_types[index + offset] == NULL) {
      tra_ffic_error_set(error, "Unsupported callback argument type");
      return 0;
    }
  }
  return 1;
}

static inline int tra_ffic_buffer_view_is_valid(
    const tra_ffic_buffer_view *view) {
  return view != NULL && (view->data != NULL || view->size == 0u);
}

/**
 * Initializes a deferred task queue.
 *
 * @param queue The queue to initialize.
 * @param drain_finalization_callback Optional notification callback. Pass NULL
 * to disable notifications.
 * @param drain_finalization_state User state passed to the notification
 * callback. May be NULL.
 * @return Non-zero on success.
 */
static inline int tra_ffic_task_queue_init(
    tra_ffic_task_queue *queue,
    tra_ffic_task_drain_finalization_callback drain_finalization_callback,
    void *drain_finalization_state) {
  if (queue == NULL) {
    return 0;
  }
  memset(queue, 0, sizeof(*queue));
  if (!tra_ffic_mutex_init(&queue->mutex)) {
    return 0;
  }
  queue->drain_finalization_callback = drain_finalization_callback;
  queue->drain_finalization_state = drain_finalization_state;
  queue->initialized = true;
  return 1;
}

/** Enqueues one task for later tra_ffic_task_drain_finalization execution. */
static inline int tra_ffic_task_queue_schedule(
    tra_ffic_task_queue *queue,
    tra_ffic_task_function task,
    void *task_data) {
  tra_ffic_task_drain_finalization_callback drain_finalization_callback = NULL;
  void *drain_finalization_state = NULL;
  tra_ffic_task_node *node = NULL;
  if (queue == NULL || !queue->initialized || task == NULL) {
    return 0;
  }
  node = (tra_ffic_task_node *)calloc(1u, sizeof(*node));
  if (node == NULL) {
    return 0;
  }
  node->task = task;
  node->task_data = task_data;
  tra_ffic_mutex_lock(&queue->mutex);
  if (queue->tail == NULL) {
    queue->head = node;
    queue->tail = node;
  } else {
    queue->tail->next = node;
    queue->tail = node;
  }
  drain_finalization_callback = queue->drain_finalization_callback;
  drain_finalization_state = queue->drain_finalization_state;
  tra_ffic_mutex_unlock(&queue->mutex);
  if (drain_finalization_callback != NULL) {
    drain_finalization_callback(queue, drain_finalization_state);
  }
  return 1;
}

/** Adapter that lets a tra_ffic_task_queue act as a side scheduler. */
static inline int tra_ffic_task_queue_schedule_callback(
    void *schedule_data,
    tra_ffic_task_function task,
    void *task_data) {
  return tra_ffic_task_queue_schedule((tra_ffic_task_queue *)schedule_data,
                                         task, task_data);
}

/** Runs all tasks currently queued, including tasks added by earlier tasks. */
static inline void tra_ffic_task_drain_finalization(
    tra_ffic_task_queue *queue) {
  for (;;) {
    tra_ffic_task_node *node = NULL;
    if (queue == NULL || !queue->initialized) {
      return;
    }
    tra_ffic_mutex_lock(&queue->mutex);
    node = queue->head;
    if (node != NULL) {
      queue->head = node->next;
      if (queue->head == NULL) {
        queue->tail = NULL;
      }
    }
    tra_ffic_mutex_unlock(&queue->mutex);
    if (node == NULL) {
      return;
    }
    node->task(node->task_data);
    free(node);
  }
}

/** Destroys a task queue and discards any tasks that were not drained. */
static inline void tra_ffic_task_queue_destroy(
    tra_ffic_task_queue *queue) {
  tra_ffic_task_node *node = NULL;
  if (queue == NULL || !queue->initialized) {
    return;
  }
  tra_ffic_mutex_lock(&queue->mutex);
  node = queue->head;
  queue->head = NULL;
  queue->tail = NULL;
  tra_ffic_mutex_unlock(&queue->mutex);
  while (node != NULL) {
    tra_ffic_task_node *next = node->next;
    free(node);
    node = next;
  }
  tra_ffic_mutex_destroy(&queue->mutex);
  queue->initialized = false;
}

static inline tra_ffic_registry_entry *tra_ffic_side_find_locked(
    tra_ffic_side *side,
    tra_ffic_native_function raw) {
  tra_ffic_registry_entry *entry = NULL;
  const uintptr_t key = tra_ffic_function_key(raw);
  if (side == NULL) {
    return NULL;
  }
  entry = side->entries;
  while (entry != NULL) {
    if (tra_ffic_function_key(entry->raw) == key) {
      return entry;
    }
    entry = entry->next;
  }
  return NULL;
}

static inline void tra_ffic_side_remove_locked(
    tra_ffic_side *side,
    tra_ffic_registry_entry *target) {
  tra_ffic_registry_entry **cursor = NULL;
  if (side == NULL || target == NULL) {
    return;
  }
  cursor = &side->entries;
  while (*cursor != NULL) {
    if (*cursor == target) {
      *cursor = target->next;
      target->next = NULL;
      return;
    }
    cursor = &(*cursor)->next;
  }
}

static inline void tra_ffic_global_registry_add(
    tra_ffic_registry_entry *entry) {
  if (entry == NULL) {
    return;
  }
  tra_ffic_static_mutex_lock(&tra_ffic_global_registry_mutex);
  entry->global_next = tra_ffic_global_registry_entries;
  tra_ffic_global_registry_entries = entry;
  tra_ffic_static_mutex_unlock(&tra_ffic_global_registry_mutex);
}

static inline void tra_ffic_global_registry_remove(
    tra_ffic_registry_entry *target) {
  tra_ffic_registry_entry **cursor = NULL;
  if (target == NULL) {
    return;
  }
  tra_ffic_static_mutex_lock(&tra_ffic_global_registry_mutex);
  cursor = &tra_ffic_global_registry_entries;
  while (*cursor != NULL) {
    if (*cursor == target) {
      *cursor = target->global_next;
      target->global_next = NULL;
      break;
    }
    cursor = &(*cursor)->global_next;
  }
  tra_ffic_static_mutex_unlock(&tra_ffic_global_registry_mutex);
}

static inline tra_ffic_registry_entry *tra_ffic_global_registry_find(
    tra_ffic_native_function raw,
    const tra_ffic_signature *signature) {
  tra_ffic_registry_entry *entry = NULL;
  if (raw == NULL) {
    return NULL;
  }
  tra_ffic_static_mutex_lock(&tra_ffic_global_registry_mutex);
  entry = tra_ffic_global_registry_entries;
  while (entry != NULL) {
    if (tra_ffic_function_key(entry->raw) == tra_ffic_function_key(raw) &&
        !entry->destroy_scheduled &&
        (signature == NULL ||
         tra_ffic_signature_equals(entry->signature, signature))) {
      tra_ffic_static_mutex_unlock(&tra_ffic_global_registry_mutex);
      return entry;
    }
    entry = entry->global_next;
  }
  tra_ffic_static_mutex_unlock(&tra_ffic_global_registry_mutex);
  return NULL;
}

static inline tra_ffic_registry_entry *
tra_ffic_global_registry_find_logical(
    tra_ffic_native_function raw,
    const tra_ffic_signature *signature) {
  tra_ffic_registry_entry *entry = NULL;
  if (raw == NULL || signature == NULL) {
    return NULL;
  }
  tra_ffic_static_mutex_lock(&tra_ffic_global_registry_mutex);
  entry = tra_ffic_global_registry_entries;
  while (entry != NULL) {
    if (tra_ffic_function_key(entry->raw) == tra_ffic_function_key(raw) &&
        !entry->destroy_scheduled &&
        tra_ffic_signature_logically_equals(entry->signature, signature)) {
      tra_ffic_static_mutex_unlock(&tra_ffic_global_registry_mutex);
      return entry;
    }
    entry = entry->global_next;
  }
  tra_ffic_static_mutex_unlock(&tra_ffic_global_registry_mutex);
  return NULL;
}

static inline void tra_ffic_registry_entry_destroy(void *task_data) {
  tra_ffic_registry_entry *entry = (tra_ffic_registry_entry *)task_data;
  if (entry == NULL) {
    return;
  }
  tra_ffic_closure_free_tracked(entry->closure);
  entry->closure = NULL;
  entry->closure_code = NULL;
  if (entry->finalize_user_data != NULL) {
    entry->finalize_user_data(entry->user_data);
  }
  tra_ffic_signature_destroy(entry->signature);
  if (entry->has_completion_return_type) {
    tra_ffic_type_destroy(&entry->completion_return_type);
  }
  free(entry->ffi_arg_types);
  free(entry->callback_arg_types);
  free(entry);
}

static inline void tra_ffic_registry_entry_schedule_destroy(
    tra_ffic_registry_entry *entry) {
  tra_ffic_side *side = NULL;
  if (entry == NULL || entry->destroy_scheduled) {
    return;
  }
  side = entry->owner_side;
  entry->destroy_scheduled = true;
  if (side != NULL && side->schedule != NULL &&
      side->schedule(side->schedule_data, tra_ffic_registry_entry_destroy,
                     entry)) {
    return;
  }
  tra_ffic_registry_entry_destroy(entry);
}

static inline tra_ffic_registry_entry *tra_ffic_entry_add_active(
    const tra_ffic_function_ref *ref,
    tra_ffic_error *error) {
  tra_ffic_registry_entry *entry = NULL;
  if (ref == NULL || ref->raw == NULL || ref->owner_side == NULL) {
    tra_ffic_error_set(error, "Function reference is null");
    return NULL;
  }
  tra_ffic_mutex_lock(&ref->owner_side->mutex);
  entry = tra_ffic_side_find_locked(ref->owner_side, ref->raw);
  if (entry == NULL || entry->destroy_scheduled) {
    tra_ffic_mutex_unlock(&ref->owner_side->mutex);
    tra_ffic_error_set(error, "Function reference is unknown");
    return NULL;
  }
  if (!tra_ffic_signature_equals(entry->signature, ref->signature)) {
    tra_ffic_mutex_unlock(&ref->owner_side->mutex);
    tra_ffic_error_set(error, "Function reference signature mismatch");
    return NULL;
  }
  entry->active_call_count += 1u;
  tra_ffic_mutex_unlock(&ref->owner_side->mutex);
  return entry;
}

static inline tra_ffic_registry_entry *tra_ffic_entry_add_active_entry(
    tra_ffic_registry_entry *entry,
    tra_ffic_error *error) {
  tra_ffic_side *side = NULL;
  if (entry == NULL || entry->owner_side == NULL || entry->raw == NULL) {
    tra_ffic_error_set(error, "Function reference is null");
    return NULL;
  }
  side = entry->owner_side;
  tra_ffic_mutex_lock(&side->mutex);
  if (entry->destroy_scheduled) {
    tra_ffic_mutex_unlock(&side->mutex);
    tra_ffic_error_set(error, "Function reference is unknown");
    return NULL;
  }
  entry->active_call_count += 1u;
  tra_ffic_mutex_unlock(&side->mutex);
  return entry;
}

static inline tra_ffic_registry_entry *tra_ffic_entry_add_active_by_raw(
    tra_ffic_native_function raw,
    const tra_ffic_signature *signature,
    tra_ffic_error *error) {
  tra_ffic_registry_entry *entry =
      tra_ffic_global_registry_find(raw, signature);
  if (entry == NULL) {
    tra_ffic_error_set(error, "Function reference is unknown");
    return NULL;
  }
  return tra_ffic_entry_add_active_entry(entry, error);
}

static inline void tra_ffic_entry_release_active(
    tra_ffic_registry_entry *entry) {
  bool should_destroy = false;
  tra_ffic_side *side = NULL;
  if (entry == NULL || entry->owner_side == NULL) {
    return;
  }
  side = entry->owner_side;
  tra_ffic_mutex_lock(&side->mutex);
  if (entry->active_call_count > 0u) {
    entry->active_call_count -= 1u;
  }
  if (entry->ref_count == 0u && entry->active_call_count == 0u &&
      !entry->destroy_scheduled) {
    tra_ffic_side_remove_locked(side, entry);
    tra_ffic_global_registry_remove(entry);
    should_destroy = true;
  }
  tra_ffic_mutex_unlock(&side->mutex);
  if (should_destroy) {
    tra_ffic_registry_entry_schedule_destroy(entry);
  }
}

static inline tra_ffic_registry_entry *tra_ffic_find_owner_entry(
    tra_ffic_side *hint,
    tra_ffic_native_function raw,
    const tra_ffic_signature *signature) {
  tra_ffic_registry_entry *entry = NULL;
  tra_ffic_side *sides[2];
  size_t index = 0u;
  sides[0] = hint;
  sides[1] = hint != NULL ? hint->peer : NULL;
  for (index = 0u; index < 2u; ++index) {
    tra_ffic_side *side = sides[index];
    if (side == NULL) {
      continue;
    }
    tra_ffic_mutex_lock(&side->mutex);
    entry = tra_ffic_side_find_locked(side, raw);
    if (entry != NULL && !entry->destroy_scheduled &&
        tra_ffic_signature_equals(entry->signature, signature)) {
      tra_ffic_mutex_unlock(&side->mutex);
      return entry;
    }
    tra_ffic_mutex_unlock(&side->mutex);
  }
  entry = tra_ffic_global_registry_find(raw, signature);
  return entry;
}

/** Retains a registered function pointer beyond the current callback. */
static inline int tra_ffic_function_retain_impl(
    tra_ffic_native_function function,
    tra_ffic_error *error) {
  tra_ffic_registry_entry *entry = NULL;
  tra_ffic_error_clear(error);
  if (function == NULL) {
    return 1;
  }
  entry = tra_ffic_global_registry_find(function, NULL);
  if (entry == NULL || entry->owner_side == NULL) {
    tra_ffic_error_set(error, "Function reference is unknown");
    return 0;
  }
  tra_ffic_mutex_lock(&entry->owner_side->mutex);
  if (entry->destroy_scheduled ||
      tra_ffic_function_key(entry->raw) != tra_ffic_function_key(function)) {
    tra_ffic_mutex_unlock(&entry->owner_side->mutex);
    tra_ffic_error_set(error, "Function reference is unknown");
    return 0;
  }
  entry->ref_count += 1u;
  tra_ffic_mutex_unlock(&entry->owner_side->mutex);
  return 1;
}

/** Releases one retain for a registered function pointer. */
static inline int tra_ffic_function_release_impl(
    tra_ffic_native_function function,
    tra_ffic_error *error) {
  tra_ffic_registry_entry *entry = NULL;
  bool should_destroy = false;
  tra_ffic_error_clear(error);
  if (function == NULL) {
    return 1;
  }
  entry = tra_ffic_global_registry_find(function, NULL);
  if (entry == NULL || entry->owner_side == NULL) {
    tra_ffic_error_set(error, "Function reference is unknown");
    return 0;
  }
  tra_ffic_mutex_lock(&entry->owner_side->mutex);
  if (entry->destroy_scheduled ||
      tra_ffic_function_key(entry->raw) != tra_ffic_function_key(function)) {
    tra_ffic_mutex_unlock(&entry->owner_side->mutex);
    tra_ffic_error_set(error, "Function reference is unknown");
    return 0;
  }
  if (entry->ref_count == 0u) {
    tra_ffic_mutex_unlock(&entry->owner_side->mutex);
    tra_ffic_error_set(error, "Function reference has no retain count");
    return 0;
  }
  entry->ref_count -= 1u;
  if (entry->ref_count == 0u && entry->active_call_count == 0u) {
    tra_ffic_side_remove_locked(entry->owner_side, entry);
    tra_ffic_global_registry_remove(entry);
    should_destroy = true;
  }
  tra_ffic_mutex_unlock(&entry->owner_side->mutex);
  if (should_destroy) {
    tra_ffic_registry_entry_schedule_destroy(entry);
  }
  return 1;
}

static inline void tra_ffic_side_add_entry(
    tra_ffic_side *side,
    tra_ffic_registry_entry *entry);

typedef union tra_ffic_arg_storage {
  tra_ffic_completion completion_value;
  void *pointer_value;
  uint8_t bool_value;
  int8_t int8_value;
  uint8_t uint8_value;
  int16_t int16_value;
  uint16_t uint16_value;
  int32_t int32_value;
  uint32_t uint32_value;
  int64_t int64_value;
  uint64_t uint64_value;
  float float_value;
  double double_value;
  tra_ffic_buffer_view buffer_view_value;
  const char *string_value;
  tra_ffic_native_function function_value;
} tra_ffic_arg_storage;

static inline void *tra_ffic_arg_storage_value_address(
    tra_ffic_type_kind kind,
    tra_ffic_arg_storage *storage) {
  if (storage == NULL) {
    return NULL;
  }
  switch (kind) {
    case TRA_FFIC_TYPE_BOOL:
      return &storage->bool_value;
    case TRA_FFIC_TYPE_INT8:
      return &storage->int8_value;
    case TRA_FFIC_TYPE_UINT8:
      return &storage->uint8_value;
    case TRA_FFIC_TYPE_INT16:
      return &storage->int16_value;
    case TRA_FFIC_TYPE_UINT16:
      return &storage->uint16_value;
    case TRA_FFIC_TYPE_INT32:
      return &storage->int32_value;
    case TRA_FFIC_TYPE_UINT32:
      return &storage->uint32_value;
    case TRA_FFIC_TYPE_INT64:
      return &storage->int64_value;
    case TRA_FFIC_TYPE_UINT64:
      return &storage->uint64_value;
    case TRA_FFIC_TYPE_FLOAT:
      return &storage->float_value;
    case TRA_FFIC_TYPE_DOUBLE:
      return &storage->double_value;
    case TRA_FFIC_TYPE_BUFFER_VIEW:
      return &storage->buffer_view_value;
    case TRA_FFIC_TYPE_STRING:
      return &storage->string_value;
    case TRA_FFIC_TYPE_POINTER:
      return &storage->pointer_value;
    case TRA_FFIC_TYPE_FUNCTION:
      return &storage->function_value;
    case TRA_FFIC_TYPE_VOID:
      return NULL;
  }
  return NULL;
}

static inline int tra_ffic_copy_arg_storage_to_address(
    const tra_ffic_type *type,
    const tra_ffic_arg_storage *storage,
    void *target) {
  if (type == NULL || storage == NULL ||
      (type->kind != TRA_FFIC_TYPE_VOID && target == NULL)) {
    return 0;
  }
  switch (type->kind) {
    case TRA_FFIC_TYPE_VOID:
      return 1;
    case TRA_FFIC_TYPE_BOOL:
      *(uint8_t *)target = storage->bool_value;
      return 1;
    case TRA_FFIC_TYPE_INT8:
      *(int8_t *)target = storage->int8_value;
      return 1;
    case TRA_FFIC_TYPE_UINT8:
      *(uint8_t *)target = storage->uint8_value;
      return 1;
    case TRA_FFIC_TYPE_INT16:
      *(int16_t *)target = storage->int16_value;
      return 1;
    case TRA_FFIC_TYPE_UINT16:
      *(uint16_t *)target = storage->uint16_value;
      return 1;
    case TRA_FFIC_TYPE_INT32:
      *(int32_t *)target = storage->int32_value;
      return 1;
    case TRA_FFIC_TYPE_UINT32:
      *(uint32_t *)target = storage->uint32_value;
      return 1;
    case TRA_FFIC_TYPE_INT64:
      *(int64_t *)target = storage->int64_value;
      return 1;
    case TRA_FFIC_TYPE_UINT64:
      *(uint64_t *)target = storage->uint64_value;
      return 1;
    case TRA_FFIC_TYPE_FLOAT:
      *(float *)target = storage->float_value;
      return 1;
    case TRA_FFIC_TYPE_DOUBLE:
      *(double *)target = storage->double_value;
      return 1;
    case TRA_FFIC_TYPE_BUFFER_VIEW:
      *(tra_ffic_buffer_view *)target = storage->buffer_view_value;
      return 1;
    case TRA_FFIC_TYPE_POINTER:
      *(void **)target = storage->pointer_value;
      return 1;
    case TRA_FFIC_TYPE_STRING:
      *(const char **)target = storage->string_value;
      return 1;
    case TRA_FFIC_TYPE_FUNCTION:
      *(tra_ffic_native_function *)target = storage->function_value;
      return 1;
  }
  return 0;
}

struct tra_ffic_function_adapter_state {
  tra_ffic_function_ref source_ref;
  tra_ffic_signature *source_signature;
};

static inline int tra_ffic_call_with_result(
    tra_ffic_side *caller_side,
    const tra_ffic_function_ref *target_ref,
    const tra_ffic_value *args,
    uint32_t arg_count,
    tra_ffic_result_callback result_callback,
    void *result_user_data,
    tra_ffic_error *error);

static inline void tra_ffic_function_adapter_trampoline(
    ffi_cif *cif,
    void *return_value,
    void **args,
    void *user_data);

static inline void tra_ffic_function_adapter_state_destroy(
    void *user_data) {
  tra_ffic_function_adapter_state *state =
      (tra_ffic_function_adapter_state *)user_data;
  tra_ffic_error error;
  if (state == NULL) {
    return;
  }
  if (state->source_ref.raw != NULL) {
    (void)tra_ffic_function_release_impl(state->source_ref.raw, &error);
  }
  tra_ffic_signature_destroy(state->source_signature);
  free(state);
}

static inline tra_ffic_registry_entry *
tra_ffic_create_function_adapter_entry(
    tra_ffic_side *owner_side,
    tra_ffic_registry_entry *source_entry,
    const tra_ffic_signature *expected_signature,
    bool retained,
    tra_ffic_error *error) {
  tra_ffic_registry_entry *entry = NULL;
  tra_ffic_function_adapter_state *state = NULL;
  ffi_status status = FFI_OK;
  ffi_type *native_return_type = NULL;
  if (owner_side == NULL || !owner_side->initialized || source_entry == NULL ||
      expected_signature == NULL) {
    tra_ffic_error_set(error, "Function adapter input is null");
    return NULL;
  }
  if (!tra_ffic_signature_logically_equals(source_entry->signature,
                                           expected_signature)) {
    tra_ffic_error_set(error, "Function adapter signature mismatch");
    return NULL;
  }
  if (!tra_ffic_function_retain_impl(source_entry->raw, error)) {
    return NULL;
  }
  entry = (tra_ffic_registry_entry *)calloc(1u, sizeof(*entry));
  state = (tra_ffic_function_adapter_state *)calloc(1u, sizeof(*state));
  if (entry == NULL || state == NULL) {
    free(entry);
    free(state);
    (void)tra_ffic_function_release_impl(source_entry->raw, NULL);
    tra_ffic_error_set(error, "Out of memory creating function adapter");
    return NULL;
  }
  entry->owner_side = owner_side;
  entry->signature = tra_ffic_signature_clone(expected_signature, 0u, error);
  state->source_signature =
      tra_ffic_signature_clone(source_entry->signature, 0u, error);
  if (entry->signature == NULL || state->source_signature == NULL) {
    entry->user_data = state;
    entry->finalize_user_data = tra_ffic_function_adapter_state_destroy;
    tra_ffic_registry_entry_destroy(entry);
    return NULL;
  }
  state->source_ref.raw = source_entry->raw;
  state->source_ref.owner_side = source_entry->owner_side;
  state->source_ref.signature = state->source_signature;
  entry->user_data = state;
  entry->finalize_user_data = tra_ffic_function_adapter_state_destroy;
  entry->ref_count = retained ? 1u : 0u;
  entry->active_call_count = retained ? 0u : 1u;
  native_return_type =
      tra_ffic_signature_uses_completion(entry->signature)
          ? &ffi_type_void
          : tra_ffic_get_ffi_type(entry->signature->return_type, true);
  if (native_return_type == NULL) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error, "Unsupported adapter return type");
    return NULL;
  }
  entry->ffi_arg_count = tra_ffic_public_ffi_arg_count(entry->signature);
  if (entry->ffi_arg_count > 0u) {
    entry->ffi_arg_types =
        (ffi_type **)calloc(entry->ffi_arg_count, sizeof(*entry->ffi_arg_types));
    if (entry->ffi_arg_types == NULL) {
      tra_ffic_registry_entry_destroy(entry);
      tra_ffic_error_set(error, "Out of memory creating adapter ffi table");
      return NULL;
    }
  }
  if (!tra_ffic_fill_public_ffi_arg_types(entry->signature,
                                          entry->ffi_arg_types, error)) {
    tra_ffic_registry_entry_destroy(entry);
    return NULL;
  }
  status = ffi_prep_cif(&entry->cif, FFI_DEFAULT_ABI, entry->ffi_arg_count,
                        native_return_type, entry->ffi_arg_types);
  if (status != FFI_OK) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error, "ffi_prep_cif failed for function adapter");
    return NULL;
  }
  entry->closure = tra_ffic_closure_alloc_tracked(&entry->closure_code);
  if (entry->closure == NULL || entry->closure_code == NULL) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error, "ffi_closure_alloc failed for function adapter");
    return NULL;
  }
  status = ffi_prep_closure_loc(entry->closure, &entry->cif,
                                tra_ffic_function_adapter_trampoline, entry,
                                entry->closure_code);
  if (status != FFI_OK) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error,
                       "ffi_prep_closure_loc failed for function adapter");
    return NULL;
  }
  entry->raw = tra_ffic_native_from_code(entry->closure_code);
  tra_ffic_side_add_entry(owner_side, entry);
  return entry;
}

static inline tra_ffic_registry_entry *
tra_ffic_add_active_or_adapter_by_raw(
    tra_ffic_side *adapter_owner_side,
    tra_ffic_native_function raw,
    const tra_ffic_signature *expected_signature,
    bool retained_adapter,
    tra_ffic_error *error) {
  tra_ffic_registry_entry *entry = NULL;
  tra_ffic_registry_entry *source_entry = NULL;
  if (raw == NULL) {
    return NULL;
  }
  entry = tra_ffic_entry_add_active_by_raw(raw, expected_signature, NULL);
  if (entry != NULL) {
    return entry;
  }
  source_entry =
      tra_ffic_global_registry_find_logical(raw, expected_signature);
  if (source_entry == NULL) {
    tra_ffic_error_set(error, "Function reference is unknown");
    return NULL;
  }
  if (tra_ffic_signature_equals(source_entry->signature,
                                expected_signature)) {
    return tra_ffic_entry_add_active_entry(source_entry, error);
  }
  return tra_ffic_create_function_adapter_entry(
      adapter_owner_side != NULL ? adapter_owner_side : source_entry->owner_side,
      source_entry, expected_signature, retained_adapter, error);
}

static inline int tra_ffic_store_function_for_expected(
    tra_ffic_side *adapter_owner_side,
    const tra_ffic_type *expected_type,
    tra_ffic_native_function raw_function,
    tra_ffic_arg_storage *storage,
    tra_ffic_registry_entry **protected_entry,
    bool retained_adapter,
    tra_ffic_error *error) {
  tra_ffic_registry_entry *entry = NULL;
  if (protected_entry != NULL) {
    *protected_entry = NULL;
  }
  if (expected_type == NULL || storage == NULL ||
      expected_type->kind != TRA_FFIC_TYPE_FUNCTION) {
    tra_ffic_error_set(error, "Function storage input is invalid");
    return 0;
  }
  if (raw_function == NULL) {
    storage->function_value = NULL;
    return 1;
  }
  entry = tra_ffic_add_active_or_adapter_by_raw(
      adapter_owner_side, raw_function, expected_type->function_signature,
      retained_adapter, error);
  if (entry == NULL) {
    return 0;
  }
  storage->function_value = entry->raw;
  if (retained_adapter && entry->raw != raw_function) {
    tra_ffic_entry_release_active(entry);
  } else if (protected_entry != NULL) {
    *protected_entry = entry;
  } else {
    tra_ffic_entry_release_active(entry);
  }
  return 1;
}

static inline int tra_ffic_decode_raw_arg_to_storage(
    tra_ffic_side *adapter_owner_side,
    const tra_ffic_type *type,
    void *raw_arg,
    tra_ffic_arg_storage *storage,
    tra_ffic_value *raw_value,
    tra_ffic_registry_entry **protected_entry,
    tra_ffic_error *error) {
  if (protected_entry != NULL) {
    *protected_entry = NULL;
  }
  if (type == NULL || raw_arg == NULL || storage == NULL) {
    tra_ffic_error_set(error, "Argument pointer is null");
    return 0;
  }
  switch (type->kind) {
    case TRA_FFIC_TYPE_BOOL:
      storage->bool_value =
          *(uint8_t *)raw_arg != (uint8_t)0u ? (uint8_t)1u : (uint8_t)0u;
      if (raw_value != NULL) {
        *raw_value = tra_ffic_value_bool(storage->bool_value != 0u);
      }
      return 1;
    case TRA_FFIC_TYPE_INT8:
      storage->int8_value = *(int8_t *)raw_arg;
      if (raw_value != NULL) {
        *raw_value = tra_ffic_value_int8(storage->int8_value);
      }
      return 1;
    case TRA_FFIC_TYPE_UINT8:
      storage->uint8_value = *(uint8_t *)raw_arg;
      if (raw_value != NULL) {
        *raw_value = tra_ffic_value_uint8(storage->uint8_value);
      }
      return 1;
    case TRA_FFIC_TYPE_INT16:
      storage->int16_value = *(int16_t *)raw_arg;
      if (raw_value != NULL) {
        *raw_value = tra_ffic_value_int16(storage->int16_value);
      }
      return 1;
    case TRA_FFIC_TYPE_UINT16:
      storage->uint16_value = *(uint16_t *)raw_arg;
      if (raw_value != NULL) {
        *raw_value = tra_ffic_value_uint16(storage->uint16_value);
      }
      return 1;
    case TRA_FFIC_TYPE_INT32:
      storage->int32_value = *(int32_t *)raw_arg;
      if (raw_value != NULL) {
        *raw_value = tra_ffic_value_int32(storage->int32_value);
      }
      return 1;
    case TRA_FFIC_TYPE_UINT32:
      storage->uint32_value = *(uint32_t *)raw_arg;
      if (raw_value != NULL) {
        *raw_value = tra_ffic_value_uint32(storage->uint32_value);
      }
      return 1;
    case TRA_FFIC_TYPE_INT64:
      storage->int64_value = *(int64_t *)raw_arg;
      if (raw_value != NULL) {
        *raw_value = tra_ffic_value_int64(storage->int64_value);
      }
      return 1;
    case TRA_FFIC_TYPE_UINT64:
      storage->uint64_value = *(uint64_t *)raw_arg;
      if (raw_value != NULL) {
        *raw_value = tra_ffic_value_uint64(storage->uint64_value);
      }
      return 1;
    case TRA_FFIC_TYPE_FLOAT:
      storage->float_value = *(float *)raw_arg;
      if (raw_value != NULL) {
        *raw_value = tra_ffic_value_float(storage->float_value);
      }
      return 1;
    case TRA_FFIC_TYPE_DOUBLE:
      storage->double_value = *(double *)raw_arg;
      if (raw_value != NULL) {
        *raw_value = tra_ffic_value_double(storage->double_value);
      }
      return 1;
    case TRA_FFIC_TYPE_BUFFER_VIEW:
      storage->buffer_view_value = *(tra_ffic_buffer_view *)raw_arg;
      if (!tra_ffic_buffer_view_is_valid(&storage->buffer_view_value)) {
        tra_ffic_error_set(error, "Invalid buffer view");
        return 0;
      }
      if (raw_value != NULL) {
        *raw_value = tra_ffic_value_buffer_view(
            storage->buffer_view_value.data, storage->buffer_view_value.size);
      }
      return 1;
    case TRA_FFIC_TYPE_POINTER:
      storage->pointer_value = *(void **)raw_arg;
      if (raw_value != NULL) {
        *raw_value = tra_ffic_value_pointer(storage->pointer_value);
      }
      return 1;
    case TRA_FFIC_TYPE_STRING:
      storage->string_value = *(const char **)raw_arg;
      if (raw_value != NULL) {
        *raw_value = tra_ffic_value_string(storage->string_value);
      }
      return 1;
    case TRA_FFIC_TYPE_FUNCTION: {
      tra_ffic_native_function raw_function =
          *(tra_ffic_native_function *)raw_arg;
      if (!tra_ffic_store_function_for_expected(
              adapter_owner_side, type, raw_function, storage,
              protected_entry, false, error)) {
        tra_ffic_error_set(error, "Unknown function argument");
        return 0;
      }
      if (raw_value != NULL) {
        *raw_value = tra_ffic_value_function(storage->function_value);
      }
      return 1;
    }
    case TRA_FFIC_TYPE_VOID:
      tra_ffic_error_set(error, "Void argument is invalid");
      return 0;
  }
  tra_ffic_error_set(error, "Unsupported argument type");
  return 0;
}

static inline int tra_ffic_store_value_for_type(
    tra_ffic_side *adapter_owner_side,
    const tra_ffic_type *expected_type,
    const tra_ffic_value *source,
    tra_ffic_arg_storage *storage,
    tra_ffic_registry_entry **protected_entry,
    bool retained_adapter,
    tra_ffic_error *error) {
  if (protected_entry != NULL) {
    *protected_entry = NULL;
  }
  if (expected_type == NULL || source == NULL || storage == NULL) {
    tra_ffic_error_set(error, "Value storage input is null");
    return 0;
  }
  if (source->kind != expected_type->kind) {
    tra_ffic_error_set(error, "Value type mismatch");
    return 0;
  }
  switch (expected_type->kind) {
    case TRA_FFIC_TYPE_BOOL:
      storage->bool_value = source->as.bool_value ? (uint8_t)1u : (uint8_t)0u;
      return 1;
    case TRA_FFIC_TYPE_INT8:
      storage->int8_value = source->as.int8_value;
      return 1;
    case TRA_FFIC_TYPE_UINT8:
      storage->uint8_value = source->as.uint8_value;
      return 1;
    case TRA_FFIC_TYPE_INT16:
      storage->int16_value = source->as.int16_value;
      return 1;
    case TRA_FFIC_TYPE_UINT16:
      storage->uint16_value = source->as.uint16_value;
      return 1;
    case TRA_FFIC_TYPE_INT32:
      storage->int32_value = source->as.int32_value;
      return 1;
    case TRA_FFIC_TYPE_UINT32:
      storage->uint32_value = source->as.uint32_value;
      return 1;
    case TRA_FFIC_TYPE_INT64:
      storage->int64_value = source->as.int64_value;
      return 1;
    case TRA_FFIC_TYPE_UINT64:
      storage->uint64_value = source->as.uint64_value;
      return 1;
    case TRA_FFIC_TYPE_FLOAT:
      storage->float_value = source->as.float_value;
      return 1;
    case TRA_FFIC_TYPE_DOUBLE:
      storage->double_value = source->as.double_value;
      return 1;
    case TRA_FFIC_TYPE_BUFFER_VIEW:
      if (!tra_ffic_buffer_view_is_valid(&source->as.buffer_view_value)) {
        tra_ffic_error_set(error, "Invalid buffer view");
        return 0;
      }
      storage->buffer_view_value = source->as.buffer_view_value;
      return 1;
    case TRA_FFIC_TYPE_POINTER:
      storage->pointer_value = source->as.pointer_value;
      return 1;
    case TRA_FFIC_TYPE_STRING:
      storage->string_value = source->as.string_value;
      return 1;
    case TRA_FFIC_TYPE_FUNCTION:
      return tra_ffic_store_function_for_expected(
          adapter_owner_side, expected_type, source->as.function_value,
          storage, protected_entry, retained_adapter, error);
    case TRA_FFIC_TYPE_VOID:
      storage->int32_value = 0;
      return 1;
  }
  tra_ffic_error_set(error, "Unsupported value type");
  return 0;
}

typedef struct tra_ffic_pending_call {
  tra_ffic_mutex mutex;
  tra_ffic_side *caller_side;
  tra_ffic_schedule_function schedule;
  void *schedule_data;
  tra_ffic_type return_type;
  tra_ffic_result_callback result_callback;
  void *result_user_data;
  tra_ffic_registry_entry **protected_entries;
  uint32_t protected_entry_count;
  tra_ffic_registry_entry *result_function_entry;
  ffi_cif completion_cif;
  ffi_type *completion_arg_types[2];
  ffi_closure *completion_closure;
  void *completion_code;
  tra_ffic_completion completion;
  tra_ffic_result result;
  char *result_string_storage;
  bool completion_claimed;
  bool completed;
  bool native_returned;
  bool finish_scheduled;
} tra_ffic_pending_call;

static inline void tra_ffic_pending_call_destroy(
    tra_ffic_pending_call *pending) {
  uint32_t index = 0u;
  if (pending == NULL) {
    return;
  }
  tra_ffic_closure_free_tracked(pending->completion_closure);
  for (index = 0u; index < pending->protected_entry_count; ++index) {
    tra_ffic_entry_release_active(pending->protected_entries[index]);
  }
  tra_ffic_entry_release_active(pending->result_function_entry);
  free(pending->protected_entries);
  free(pending->result_string_storage);
  tra_ffic_type_destroy(&pending->return_type);
  tra_ffic_mutex_destroy(&pending->mutex);
  free(pending);
}

static inline int tra_ffic_result_set_error(tra_ffic_result *result,
                                               const char *message) {
  if (result == NULL) {
    return 0;
  }
  result->success = false;
  result->value = tra_ffic_value_void();
  if (message == NULL) {
    message = "Native call failed";
  }
  (void)snprintf(result->error_message, sizeof(result->error_message), "%s",
                 message);
  return 1;
}

static inline int tra_ffic_copy_completion_value(
    tra_ffic_pending_call *pending,
    const void *source,
    const char *error_message) {
  tra_ffic_registry_entry *entry = NULL;
  tra_ffic_error local_error;
  tra_ffic_native_function function_value = NULL;
  const char *string_value = NULL;
  tra_ffic_error_clear(&local_error);
  if (error_message != NULL) {
    return tra_ffic_result_set_error(&pending->result, error_message);
  }
  pending->result.success = true;
  pending->result.error_message[0] = '\0';
  if (pending->return_type.kind == TRA_FFIC_TYPE_VOID) {
    pending->result.value = tra_ffic_value_void();
    return 1;
  }
  if (source == NULL) {
    return tra_ffic_result_set_error(&pending->result,
                                        "Completion result is null");
  }
  switch (pending->return_type.kind) {
    case TRA_FFIC_TYPE_BOOL:
      pending->result.value = tra_ffic_value_bool(*(const bool *)source);
      return 1;
    case TRA_FFIC_TYPE_INT8:
      pending->result.value = tra_ffic_value_int8(*(const int8_t *)source);
      return 1;
    case TRA_FFIC_TYPE_UINT8:
      pending->result.value = tra_ffic_value_uint8(*(const uint8_t *)source);
      return 1;
    case TRA_FFIC_TYPE_INT16:
      pending->result.value = tra_ffic_value_int16(*(const int16_t *)source);
      return 1;
    case TRA_FFIC_TYPE_UINT16:
      pending->result.value = tra_ffic_value_uint16(*(const uint16_t *)source);
      return 1;
    case TRA_FFIC_TYPE_INT32:
      pending->result.value = tra_ffic_value_int32(*(const int32_t *)source);
      return 1;
    case TRA_FFIC_TYPE_UINT32:
      pending->result.value = tra_ffic_value_uint32(*(const uint32_t *)source);
      return 1;
    case TRA_FFIC_TYPE_INT64:
      pending->result.value = tra_ffic_value_int64(*(const int64_t *)source);
      return 1;
    case TRA_FFIC_TYPE_UINT64:
      pending->result.value = tra_ffic_value_uint64(*(const uint64_t *)source);
      return 1;
    case TRA_FFIC_TYPE_FLOAT:
      pending->result.value = tra_ffic_value_float(*(const float *)source);
      return 1;
    case TRA_FFIC_TYPE_DOUBLE:
      pending->result.value = tra_ffic_value_double(*(const double *)source);
      return 1;
    case TRA_FFIC_TYPE_BUFFER_VIEW: {
      const tra_ffic_buffer_view *view =
          (const tra_ffic_buffer_view *)source;
      if (!tra_ffic_buffer_view_is_valid(view)) {
        return tra_ffic_result_set_error(&pending->result,
                                            "Invalid buffer view");
      }
      pending->result.value =
          tra_ffic_value_buffer_view(view->data, view->size);
      return 1;
    }
    case TRA_FFIC_TYPE_STRING:
      string_value = *(const char *const *)source;
      if (string_value == NULL) {
        pending->result_string_storage = NULL;
        pending->result.value = tra_ffic_value_string(NULL);
        return 1;
      }
      pending->result_string_storage =
          tra_ffic_string_duplicate(string_value);
      if (pending->result_string_storage == NULL) {
        return tra_ffic_result_set_error(&pending->result,
                                            "Out of memory copying string");
      }
      pending->result.value =
          tra_ffic_value_string(pending->result_string_storage);
      return 1;
    case TRA_FFIC_TYPE_POINTER:
      pending->result.value =
          tra_ffic_value_pointer(*(void *const *)source);
      return 1;
    case TRA_FFIC_TYPE_FUNCTION:
      function_value = *(tra_ffic_native_function const *)source;
      if (function_value == NULL) {
        pending->result.value = tra_ffic_value_function(NULL);
        return 1;
      }
      {
        tra_ffic_arg_storage storage;
        memset(&storage, 0, sizeof(storage));
        if (!tra_ffic_store_function_for_expected(
                pending->caller_side, &pending->return_type, function_value,
                &storage, &entry, false, &local_error)) {
          return tra_ffic_result_set_error(&pending->result,
                                           local_error.message);
        }
        pending->result_function_entry = entry;
        pending->result.value =
            tra_ffic_value_function(storage.function_value);
      }
      return 1;
    case TRA_FFIC_TYPE_VOID:
      pending->result.value = tra_ffic_value_void();
      return 1;
  }
  return tra_ffic_result_set_error(&pending->result,
                                      "Completion type is unsupported");
}

static inline void tra_ffic_pending_finish_task(void *task_data) {
  tra_ffic_pending_call *pending = (tra_ffic_pending_call *)task_data;
  tra_ffic_result result;
  if (pending == NULL) {
    return;
  }
  result = pending->result;
  if (pending->result_callback != NULL) {
    pending->result_callback(pending->result_user_data, &result);
  }
  tra_ffic_pending_call_destroy(pending);
}

static inline void tra_ffic_pending_try_finish(
    tra_ffic_pending_call *pending) {
  bool should_schedule = false;
  if (pending == NULL) {
    return;
  }
  tra_ffic_mutex_lock(&pending->mutex);
  if (pending->completed && pending->native_returned &&
      !pending->finish_scheduled) {
    pending->finish_scheduled = true;
    should_schedule = true;
  }
  tra_ffic_mutex_unlock(&pending->mutex);
  if (should_schedule) {
    if (pending->schedule == NULL ||
        !pending->schedule(pending->schedule_data,
                           tra_ffic_pending_finish_task, pending)) {
      tra_ffic_pending_finish_task(pending);
    }
  }
}

static inline void tra_ffic_complete_pending(
    tra_ffic_pending_call *pending,
    const void *result,
    const char *error_message) {
  if (pending == NULL) {
    return;
  }
  tra_ffic_mutex_lock(&pending->mutex);
  if (pending->completion_claimed) {
    tra_ffic_mutex_unlock(&pending->mutex);
    return;
  }
  pending->completion_claimed = true;
  (void)tra_ffic_copy_completion_value(pending, result, error_message);
  pending->completed = true;
  tra_ffic_mutex_unlock(&pending->mutex);
  tra_ffic_pending_try_finish(pending);
}

static inline void tra_ffic_completion_trampoline(ffi_cif *cif,
                                                     void *return_value,
                                                     void **args,
                                                     void *user_data) {
  const void *result = NULL;
  const char *error_message = NULL;
  (void)cif;
  (void)return_value;
  if (args != NULL) {
    result = *(const void **)args[0];
    error_message = *(const char **)args[1];
  }
  tra_ffic_complete_pending((tra_ffic_pending_call *)user_data, result,
                               error_message);
}

static inline tra_ffic_pending_call *tra_ffic_pending_call_create(
    tra_ffic_side *caller_side,
    const tra_ffic_type *return_type,
    tra_ffic_registry_entry **protected_entries,
    uint32_t protected_entry_count,
    tra_ffic_result_callback result_callback,
    void *result_user_data,
    tra_ffic_error *error) {
  tra_ffic_pending_call *pending = NULL;
  ffi_status status = FFI_OK;
  if (caller_side == NULL || return_type == NULL || result_callback == NULL) {
    tra_ffic_error_set(error, "Pending call input is null");
    return NULL;
  }
  pending = (tra_ffic_pending_call *)calloc(1u, sizeof(*pending));
  if (pending == NULL) {
    tra_ffic_error_set(error, "Out of memory creating pending call");
    return NULL;
  }
  if (!tra_ffic_mutex_init(&pending->mutex)) {
    free(pending);
    tra_ffic_error_set(error, "Failed to initialize pending call mutex");
    return NULL;
  }
  pending->caller_side = caller_side;
  pending->schedule = caller_side->schedule;
  pending->schedule_data = caller_side->schedule_data;
  pending->protected_entries = protected_entries;
  pending->protected_entry_count = protected_entry_count;
  pending->result_callback = result_callback;
  pending->result_user_data = result_user_data;
  pending->completion_arg_types[0] = &ffi_type_pointer;
  pending->completion_arg_types[1] = &ffi_type_pointer;
  if (!tra_ffic_type_clone(return_type, &pending->return_type, 0u, error)) {
    pending->protected_entries = NULL;
    pending->protected_entry_count = 0u;
    tra_ffic_pending_call_destroy(pending);
    return NULL;
  }
  status = ffi_prep_cif(&pending->completion_cif, FFI_DEFAULT_ABI, 2u,
                        &ffi_type_void, pending->completion_arg_types);
  if (status != FFI_OK) {
    pending->protected_entries = NULL;
    pending->protected_entry_count = 0u;
    tra_ffic_pending_call_destroy(pending);
    tra_ffic_error_set(error, "ffi_prep_cif failed for completion");
    return NULL;
  }
  pending->completion_closure =
      tra_ffic_closure_alloc_tracked(&pending->completion_code);
  if (pending->completion_closure == NULL || pending->completion_code == NULL) {
    pending->protected_entries = NULL;
    pending->protected_entry_count = 0u;
    tra_ffic_pending_call_destroy(pending);
    tra_ffic_error_set(error, "ffi_closure_alloc failed for completion");
    return NULL;
  }
  status = ffi_prep_closure_loc(pending->completion_closure,
                                &pending->completion_cif,
                                tra_ffic_completion_trampoline, pending,
                                pending->completion_code);
  if (status != FFI_OK) {
    pending->protected_entries = NULL;
    pending->protected_entry_count = 0u;
    tra_ffic_pending_call_destroy(pending);
    tra_ffic_error_set(error,
                          "ffi_prep_closure_loc failed for completion");
    return NULL;
  }
  pending->completion = tra_ffic_completion_from_code(
      pending->completion_code);
  return pending;
}

/**
 * Initializes two sides as one same-process A/B marshaling pair.
 *
 * Both sides use the same scheduler. schedule must defer task execution until
 * after the current libffi trampoline returns.
 */
static inline int tra_ffic_side_init_pair(
    tra_ffic_side *side_a,
    tra_ffic_side *side_b,
    tra_ffic_schedule_function schedule,
    void *schedule_data,
    tra_ffic_error *error) {
  tra_ffic_error_clear(error);
  if (side_a == NULL || side_b == NULL || schedule == NULL) {
    tra_ffic_error_set(error, "Side init input is null");
    return 0;
  }
  memset(side_a, 0, sizeof(*side_a));
  memset(side_b, 0, sizeof(*side_b));
  if (!tra_ffic_mutex_init(&side_a->mutex)) {
    tra_ffic_error_set(error, "Failed to initialize side A mutex");
    return 0;
  }
  if (!tra_ffic_mutex_init(&side_b->mutex)) {
    tra_ffic_mutex_destroy(&side_a->mutex);
    tra_ffic_error_set(error, "Failed to initialize side B mutex");
    return 0;
  }
  side_a->peer = side_b;
  side_b->peer = side_a;
  side_a->schedule = schedule;
  side_b->schedule = schedule;
  side_a->schedule_data = schedule_data;
  side_b->schedule_data = schedule_data;
  side_a->initialized = true;
  side_b->initialized = true;
  return 1;
}

static inline void tra_ffic_store_zero_return(const tra_ffic_type *return_type,
                                              void *return_value) {
  tra_ffic_buffer_view empty_view;
  if (return_type == NULL || return_value == NULL) {
    return;
  }
  switch (return_type->kind) {
    case TRA_FFIC_TYPE_VOID:
      return;
    case TRA_FFIC_TYPE_BOOL:
      *(uint8_t *)return_value = 0u;
      return;
    case TRA_FFIC_TYPE_INT8:
      *(int8_t *)return_value = 0;
      return;
    case TRA_FFIC_TYPE_UINT8:
      *(uint8_t *)return_value = 0u;
      return;
    case TRA_FFIC_TYPE_INT16:
      *(int16_t *)return_value = 0;
      return;
    case TRA_FFIC_TYPE_UINT16:
      *(uint16_t *)return_value = 0u;
      return;
    case TRA_FFIC_TYPE_INT32:
      *(int32_t *)return_value = 0;
      return;
    case TRA_FFIC_TYPE_UINT32:
      *(uint32_t *)return_value = 0u;
      return;
    case TRA_FFIC_TYPE_INT64:
      *(int64_t *)return_value = 0;
      return;
    case TRA_FFIC_TYPE_UINT64:
      *(uint64_t *)return_value = 0u;
      return;
    case TRA_FFIC_TYPE_FLOAT:
      *(float *)return_value = 0.0f;
      return;
    case TRA_FFIC_TYPE_DOUBLE:
      *(double *)return_value = 0.0;
      return;
    case TRA_FFIC_TYPE_BUFFER_VIEW:
      empty_view.data = NULL;
      empty_view.size = 0u;
      *(tra_ffic_buffer_view *)return_value = empty_view;
      return;
    case TRA_FFIC_TYPE_POINTER:
    case TRA_FFIC_TYPE_STRING:
    case TRA_FFIC_TYPE_FUNCTION:
      *(void **)return_value = NULL;
      return;
  }
}

static inline void tra_ffic_closed_trampoline(ffi_cif *cif,
                                                 void *return_value,
                                                 void **args,
                                                 void *user_data) {
  tra_ffic_registry_entry *entry =
      (tra_ffic_registry_entry *)user_data;
  tra_ffic_registry_entry *active_entry = NULL;
  tra_ffic_registry_entry **protected_entries = NULL;
  uint32_t protected_count = 0u;
  tra_ffic_completion completion = NULL;
  tra_ffic_arg_storage *storages = NULL;
  void **callback_values = NULL;
  void **callback_arg_list = NULL;
  void *callback_arg_list_value = NULL;
  void *const *public_arg_list = NULL;
  tra_ffic_value *raw_values = NULL;
  uint32_t index = 0u;
  uint32_t callback_offset = 0u;
  uint32_t logical_storage_offset = 0u;
  uint32_t native_arg_offset = 0u;
  uint32_t storage_count = 0u;
  bool uses_completion_abi = false;
  void *callback_return_value = NULL;
  tra_ffic_error local_error;
  tra_ffic_error_clear(&local_error);
  (void)cif;
  if (entry == NULL ||
      (entry->callback == NULL && entry->raw_closure_callback == NULL) ||
      entry->signature == NULL || args == NULL) {
    return;
  }
  uses_completion_abi =
      entry->signature->abi == TRA_FFIC_SIGNATURE_ABI_COMPLETION;
  native_arg_offset = uses_completion_abi ? 1u : 0u;
  logical_storage_offset = (uses_completion_abi ? 1u : 0u) +
                           (entry->passes_closure_state ? 1u : 0u);
  storage_count = logical_storage_offset + entry->signature->arg_count;
  if (!uses_completion_abi) {
    tra_ffic_store_zero_return(entry->signature->return_type, return_value);
  }
  active_entry = tra_ffic_entry_add_active_entry(entry, NULL);
  if (active_entry == NULL) {
    return;
  }
  if (uses_completion_abi) {
    completion = *(tra_ffic_completion *)args[0];
  }
  if (entry->signature->argument_passing ==
      TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST) {
    const uint32_t pointer_arg_index = uses_completion_abi ? 1u : 0u;
    public_arg_list = *(void *const **)args[pointer_arg_index];
    if (entry->signature->arg_count > 0u && public_arg_list == NULL) {
      if (completion != NULL) {
        completion(NULL, "Pointer-list argument table is null");
      }
      goto cleanup;
    }
  }
  if (storage_count > 0u) {
    storages =
        (tra_ffic_arg_storage *)calloc(storage_count, sizeof(*storages));
  }
  if (entry->raw_closure_callback == NULL && entry->callback_arg_count > 0u) {
    callback_values =
        (void **)calloc(entry->callback_arg_count, sizeof(*callback_values));
  }
  if (entry->raw_closure_callback == NULL &&
      entry->callback_argument_passing ==
          TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST &&
      entry->signature->arg_count > 0u) {
    callback_arg_list =
        (void **)calloc(entry->signature->arg_count, sizeof(*callback_arg_list));
  }
  if (entry->signature->arg_count > 0u) {
    protected_entries = (tra_ffic_registry_entry **)calloc(
        entry->signature->arg_count, sizeof(*protected_entries));
    if (entry->raw_closure_callback != NULL) {
      raw_values = (tra_ffic_value *)calloc(entry->signature->arg_count,
                                            sizeof(*raw_values));
    }
  }
  if ((storage_count > 0u && storages == NULL) ||
      (entry->raw_closure_callback == NULL && entry->callback_arg_count > 0u &&
       callback_values == NULL) ||
      (entry->raw_closure_callback == NULL &&
       entry->callback_argument_passing ==
           TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST &&
       entry->signature->arg_count > 0u && callback_arg_list == NULL) ||
      (entry->signature->arg_count > 0u && protected_entries == NULL) ||
      (entry->signature->arg_count > 0u &&
       entry->raw_closure_callback != NULL && raw_values == NULL)) {
    if (completion != NULL) {
      completion(NULL, "Out of memory decoding arguments");
    }
    goto cleanup;
  }
  if (uses_completion_abi) {
    storages[callback_offset].completion_value = completion;
    if (callback_values != NULL) {
      callback_values[callback_offset] =
          &storages[callback_offset].completion_value;
    }
    callback_offset += 1u;
  }
  if (entry->passes_closure_state) {
    storages[callback_offset].pointer_value = entry->user_data;
    if (callback_values != NULL) {
      callback_values[callback_offset] =
          &storages[callback_offset].pointer_value;
    }
    callback_offset += 1u;
  }
  if (entry->raw_closure_callback == NULL &&
      entry->callback_argument_passing ==
          TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST) {
    callback_arg_list_value = callback_arg_list;
    callback_values[callback_offset] = &callback_arg_list_value;
  }
  for (index = 0u; index < entry->signature->arg_count; ++index) {
    const tra_ffic_type *type = &entry->signature->arg_types[index];
    void *raw_arg =
        entry->signature->argument_passing ==
                TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST
            ? public_arg_list[index]
            : args[index + native_arg_offset];
    tra_ffic_arg_storage *storage =
        &storages[logical_storage_offset + index];
    tra_ffic_registry_entry *protected_entry = NULL;
    void *value_address = NULL;
    if (!tra_ffic_decode_raw_arg_to_storage(
            entry->owner_side, type, raw_arg, storage,
            raw_values != NULL ? &raw_values[index] : NULL,
            &protected_entry, &local_error)) {
      if (completion != NULL) {
        completion(NULL, local_error.message);
      }
      goto cleanup;
    }
    if (protected_entry != NULL) {
      protected_entries[protected_count] = protected_entry;
      protected_count += 1u;
    }
    value_address = tra_ffic_arg_storage_value_address(type->kind, storage);
    if (entry->raw_closure_callback == NULL &&
        entry->callback_argument_passing ==
            TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST) {
      callback_arg_list[index] = value_address;
    } else if (entry->raw_closure_callback == NULL) {
      callback_values[callback_offset + index] = value_address;
    }
  }
  if (entry->raw_closure_callback != NULL) {
    entry->raw_closure_callback(completion, entry->user_data, raw_values,
                                entry->signature->arg_count);
  } else if (!uses_completion_abi) {
    if (entry->signature->return_type->kind != TRA_FFIC_TYPE_VOID) {
      callback_return_value = return_value;
    }
    ffi_call(&entry->callback_cif, FFI_FN(entry->callback),
             callback_return_value, callback_values);
  } else {
    ffi_call(&entry->callback_cif, FFI_FN(entry->callback), NULL,
             callback_values);
  }

cleanup:
  for (index = 0u; index < protected_count; ++index) {
    tra_ffic_entry_release_active(protected_entries[index]);
  }
  tra_ffic_entry_release_active(active_entry);
  free(protected_entries);
  free(raw_values);
  free(storages);
  free(callback_arg_list);
  free(callback_values);
}

static inline int tra_ffic_type_contains_function(
    const tra_ffic_type *type);

static inline int tra_ffic_signature_contains_function(
    const tra_ffic_signature *signature) {
  uint32_t index = 0u;
  if (signature == NULL) {
    return 0;
  }
  for (index = 0u; index < signature->arg_count; ++index) {
    if (tra_ffic_type_contains_function(&signature->arg_types[index])) {
      return 1;
    }
  }
  return tra_ffic_type_contains_function(signature->return_type);
}

static inline int tra_ffic_type_contains_function(
    const tra_ffic_type *type) {
  if (type == NULL) {
    return 0;
  }
  if (type->kind == TRA_FFIC_TYPE_FUNCTION) {
    return 1;
  }
  return 0;
}

static inline void tra_ffic_side_add_entry(
    tra_ffic_side *side,
    tra_ffic_registry_entry *entry) {
  tra_ffic_mutex_lock(&side->mutex);
  entry->next = side->entries;
  side->entries = entry;
  tra_ffic_mutex_unlock(&side->mutex);
  tra_ffic_global_registry_add(entry);
}

static inline int tra_ffic_side_retain_matching_function(
    tra_ffic_side *side,
    const tra_ffic_signature *signature,
    tra_ffic_user_function callback,
    bool passes_closure_state,
    void *user_data,
    tra_ffic_finalize_user_data finalize_user_data,
    tra_ffic_native_function *out_function) {
  tra_ffic_registry_entry *entry = NULL;
  if (side == NULL || out_function == NULL) {
    return 0;
  }
  tra_ffic_mutex_lock(&side->mutex);
  entry = side->entries;
  while (entry != NULL) {
    if (!entry->destroy_scheduled && entry->raw != NULL &&
        entry->callback == callback && entry->raw_closure_callback == NULL &&
        !entry->is_completion_function &&
        entry->passes_closure_state == passes_closure_state &&
        entry->user_data == user_data &&
        entry->finalize_user_data == finalize_user_data &&
        tra_ffic_signature_equals(entry->signature, signature)) {
      entry->ref_count += 1u;
      *out_function = entry->raw;
      tra_ffic_mutex_unlock(&side->mutex);
      return 1;
    }
    entry = entry->next;
  }
  tra_ffic_mutex_unlock(&side->mutex);
  return 0;
}

static inline int tra_ffic_side_create_function(
    tra_ffic_side *side,
    const tra_ffic_signature *signature,
    tra_ffic_user_function callback,
    tra_ffic_argument_passing callback_argument_passing,
    bool passes_closure_state,
    void *user_data,
    tra_ffic_finalize_user_data finalize_user_data,
    bool force_closure,
    tra_ffic_native_function *out_function,
    tra_ffic_error *error) {
  tra_ffic_registry_entry *entry = NULL;
  ffi_status status = FFI_OK;
  bool use_direct_function = false;
  bool uses_completion_abi = false;
  ffi_type *native_return_type = NULL;
  ffi_type *callback_return_type = NULL;
  tra_ffic_native_function callback_function =
      (tra_ffic_native_function)callback;
  tra_ffic_error_clear(error);
  if (side == NULL || !side->initialized || signature == NULL ||
      callback == NULL || out_function == NULL) {
    tra_ffic_error_set(error, "Create closure input is null");
    return 0;
  }
  if (!tra_ffic_is_valid_argument_passing(callback_argument_passing)) {
    tra_ffic_error_set(error, "Callback argument passing is unsupported");
    return 0;
  }
  *out_function = NULL;
  use_direct_function =
      !force_closure && !passes_closure_state &&
      signature->argument_passing == callback_argument_passing &&
      !tra_ffic_signature_contains_function(signature) &&
      tra_ffic_global_registry_find(callback_function, NULL) == NULL;
  if (tra_ffic_side_retain_matching_function(
          side, signature, callback, passes_closure_state, user_data,
          finalize_user_data, out_function)) {
    return 1;
  }
  entry = (tra_ffic_registry_entry *)calloc(1u, sizeof(*entry));
  if (entry == NULL) {
    tra_ffic_error_set(error, "Out of memory creating closure");
    return 0;
  }
  entry->owner_side = side;
  entry->signature = tra_ffic_signature_clone(signature, 0u, error);
  if (entry->signature == NULL) {
    free(entry);
    return 0;
  }
  entry->raw = callback_function;
  entry->callback = callback;
  entry->user_data = user_data;
  entry->finalize_user_data = finalize_user_data;
  entry->ref_count = 1u;
  entry->passes_closure_state = passes_closure_state;
  entry->callback_argument_passing = callback_argument_passing;
  uses_completion_abi =
      entry->signature->abi == TRA_FFIC_SIGNATURE_ABI_COMPLETION;
  native_return_type = uses_completion_abi
                           ? &ffi_type_void
                           : tra_ffic_get_ffi_type(entry->signature->return_type,
                                                   true);
  if (native_return_type == NULL) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error, "Unsupported closure return type");
    return 0;
  }
  entry->ffi_arg_count = tra_ffic_public_ffi_arg_count(entry->signature);
  if (entry->ffi_arg_count > 0u) {
    entry->ffi_arg_types =
        (ffi_type **)calloc(entry->ffi_arg_count, sizeof(*entry->ffi_arg_types));
    if (entry->ffi_arg_types == NULL) {
      tra_ffic_registry_entry_destroy(entry);
      tra_ffic_error_set(error, "Out of memory creating ffi argument table");
      return 0;
    }
  }
  if (!tra_ffic_fill_public_ffi_arg_types(entry->signature,
                                          entry->ffi_arg_types, error)) {
    tra_ffic_registry_entry_destroy(entry);
    return 0;
  }
  status = ffi_prep_cif(&entry->cif, FFI_DEFAULT_ABI, entry->ffi_arg_count,
                        native_return_type, entry->ffi_arg_types);
  if (status != FFI_OK) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error, "ffi_prep_cif failed for closure");
    return 0;
  }
  if (use_direct_function) {
    tra_ffic_side_add_entry(side, entry);
    *out_function = entry->raw;
    return 1;
  }
  callback_return_type = uses_completion_abi
                             ? &ffi_type_void
                             : tra_ffic_get_ffi_type(
                                   entry->signature->return_type, true);
  if (callback_return_type == NULL) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error, "Unsupported callback return type");
    return 0;
  }
  entry->callback_arg_count = tra_ffic_callback_ffi_arg_count(
      entry->signature, entry->callback_argument_passing,
      entry->passes_closure_state);
  if (entry->callback_arg_count > 0u) {
    entry->callback_arg_types = (ffi_type **)calloc(
        entry->callback_arg_count, sizeof(*entry->callback_arg_types));
    if (entry->callback_arg_types == NULL) {
      tra_ffic_registry_entry_destroy(entry);
      tra_ffic_error_set(error, "Out of memory creating ffi argument table");
      return 0;
    }
  }
  if (!tra_ffic_fill_callback_ffi_arg_types(
          entry->signature, entry->callback_argument_passing,
          entry->passes_closure_state, entry->callback_arg_types, error)) {
    tra_ffic_registry_entry_destroy(entry);
    return 0;
  }
  status = ffi_prep_cif(&entry->callback_cif, FFI_DEFAULT_ABI,
                        entry->callback_arg_count, callback_return_type,
                        entry->callback_arg_types);
  if (status != FFI_OK) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error, "ffi_prep_cif failed for callback");
    return 0;
  }
  entry->closure = tra_ffic_closure_alloc_tracked(&entry->closure_code);
  if (entry->closure == NULL || entry->closure_code == NULL) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error, "ffi_closure_alloc failed for closure");
    return 0;
  }
  status = ffi_prep_closure_loc(entry->closure, &entry->cif,
                                tra_ffic_closed_trampoline, entry,
                                entry->closure_code);
  if (status != FFI_OK) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error, "ffi_prep_closure_loc failed for closure");
    return 0;
  }
  entry->raw = tra_ffic_native_from_code(entry->closure_code);
  tra_ffic_side_add_entry(side, entry);
  *out_function = entry->raw;
  return 1;
}

/**
 * Creates a closed pure function pointer owned by side.
 *
 * callback has ABI void(tra_ffic_completion, ...typed_args), where typed_args
 * are described by signature.
 */
static inline int tra_ffic_side_create_pure_function_impl(
    tra_ffic_side *side,
    const tra_ffic_signature *signature,
    tra_ffic_user_function callback,
    tra_ffic_native_function *out_function,
    tra_ffic_error *error) {
  return tra_ffic_side_create_function(
      side, signature, callback, TRA_FFIC_ARGUMENT_PASSING_STACK, false, NULL,
      NULL, false, out_function, error);
}

/**
 * Creates a closed pure function pointer from a pointer-list callback.
 *
 * For completion ABI, callback has ABI
 * void(tra_ffic_completion, const void *const *args). For retval ABI,
 * callback has ABI return_type(const void *const *args).
 */
static inline int tra_ffic_side_create_pure_pointer_list_function_impl(
    tra_ffic_side *side,
    const tra_ffic_signature *signature,
    tra_ffic_user_function callback,
    tra_ffic_native_function *out_function,
    tra_ffic_error *error) {
  return tra_ffic_side_create_function(
      side, signature, callback, TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST,
      false, NULL, NULL, false, out_function, error);
}

/**
 * Creates a closed stateful function pointer owned by side.
 *
 * callback has ABI void(tra_ffic_completion, void *closure_state,
 * ...typed_args), where typed_args are described by signature.
 */
static inline int tra_ffic_side_create_closure_impl(
    tra_ffic_side *side,
    const tra_ffic_signature *signature,
    tra_ffic_user_function callback,
    void *closure_state,
    tra_ffic_finalize_user_data finalize_user_data,
    tra_ffic_native_function *out_function,
    tra_ffic_error *error) {
  return tra_ffic_side_create_function(
      side, signature, callback, TRA_FFIC_ARGUMENT_PASSING_STACK, true,
      closure_state, finalize_user_data, true, out_function, error);
}

/**
 * Creates a closed stateful function pointer from a pointer-list callback.
 *
 * For completion ABI, callback has ABI
 * void(tra_ffic_completion, void *closure_state,
 * const void *const *args). For retval ABI, callback has ABI
 * return_type(void *closure_state, const void *const *args).
 */
static inline int tra_ffic_side_create_pointer_list_closure_impl(
    tra_ffic_side *side,
    const tra_ffic_signature *signature,
    tra_ffic_user_function callback,
    void *closure_state,
    tra_ffic_finalize_user_data finalize_user_data,
    tra_ffic_native_function *out_function,
    tra_ffic_error *error) {
  return tra_ffic_side_create_function(
      side, signature, callback, TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST,
      true, closure_state, finalize_user_data, true, out_function, error);
}

/**
 * Creates a raw stateful function pointer owned by side.
 *
 * callback receives decoded tra_ffic_value arguments instead of typed C
 * parameters. Raw closures support completion ABI signatures.
 */
static inline int tra_ffic_side_create_raw_closure_impl(
    tra_ffic_side *side,
    const tra_ffic_signature *signature,
    tra_ffic_raw_closure_callback callback,
    void *closure_state,
    tra_ffic_finalize_user_data finalize_user_data,
    tra_ffic_native_function *out_function,
    tra_ffic_error *error) {
  tra_ffic_registry_entry *entry = NULL;
  ffi_status status = FFI_OK;
  tra_ffic_error_clear(error);
  if (side == NULL || !side->initialized || signature == NULL ||
      callback == NULL || out_function == NULL) {
    tra_ffic_error_set(error, "Create raw closure input is null");
    return 0;
  }
  *out_function = NULL;
  if (!tra_ffic_signature_validate(signature, 0u, error)) {
    return 0;
  }
  if (signature->abi == TRA_FFIC_SIGNATURE_ABI_RETVAL) {
    tra_ffic_error_set(error, "Raw closure does not support retval ABI");
    return 0;
  }
  entry = (tra_ffic_registry_entry *)calloc(1u, sizeof(*entry));
  if (entry == NULL) {
    tra_ffic_error_set(error, "Out of memory creating raw closure");
    return 0;
  }
  entry->owner_side = side;
  entry->signature = tra_ffic_signature_clone(signature, 0u, error);
  if (entry->signature == NULL) {
    free(entry);
    return 0;
  }
  entry->raw_closure_callback = callback;
  entry->user_data = closure_state;
  entry->finalize_user_data = finalize_user_data;
  entry->ref_count = 1u;
  entry->callback_arg_count = entry->signature->arg_count + 1u;
  entry->ffi_arg_count = tra_ffic_public_ffi_arg_count(entry->signature);
  entry->ffi_arg_types =
      (ffi_type **)calloc(entry->ffi_arg_count, sizeof(*entry->ffi_arg_types));
  if (entry->ffi_arg_types == NULL) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error, "Out of memory creating raw closure ffi table");
    return 0;
  }
  if (!tra_ffic_fill_public_ffi_arg_types(entry->signature,
                                          entry->ffi_arg_types, error)) {
    tra_ffic_registry_entry_destroy(entry);
    return 0;
  }
  status = ffi_prep_cif(&entry->cif, FFI_DEFAULT_ABI, entry->ffi_arg_count,
                        &ffi_type_void, entry->ffi_arg_types);
  if (status != FFI_OK) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error, "ffi_prep_cif failed for raw closure");
    return 0;
  }
  entry->closure = tra_ffic_closure_alloc_tracked(&entry->closure_code);
  if (entry->closure == NULL || entry->closure_code == NULL) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error, "ffi_closure_alloc failed for raw closure");
    return 0;
  }
  status = ffi_prep_closure_loc(entry->closure, &entry->cif,
                                tra_ffic_closed_trampoline, entry,
                                entry->closure_code);
  if (status != FFI_OK) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error,
                       "ffi_prep_closure_loc failed for raw closure");
    return 0;
  }
  entry->raw = tra_ffic_native_from_code(entry->closure_code);
  tra_ffic_side_add_entry(side, entry);
  *out_function = entry->raw;
  return 1;
}

typedef struct tra_ffic_completion_delivery {
  tra_ffic_registry_entry *completion_entry;
  tra_ffic_registry_entry *function_entry;
  tra_ffic_arg_storage storage;
  char *string_storage;
  tra_ffic_error error;
  bool has_error;
} tra_ffic_completion_delivery;

static inline int tra_ffic_completion_copy_value(
    tra_ffic_registry_entry *entry,
    const void *source,
    const char *error_message,
    tra_ffic_completion_delivery *delivery) {
  tra_ffic_registry_entry *function_entry = NULL;
  tra_ffic_native_function function_value = NULL;
  const char *string_value = NULL;
  tra_ffic_error local_error;
  tra_ffic_error_clear(&local_error);
  if (entry == NULL || delivery == NULL) {
    return 0;
  }
  if (error_message != NULL) {
    delivery->has_error = true;
    tra_ffic_error_set(&delivery->error, error_message);
    return 1;
  }
  switch (entry->completion_return_type.kind) {
    case TRA_FFIC_TYPE_VOID:
      return 1;
    case TRA_FFIC_TYPE_BOOL:
      if (source == NULL) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, "Completion result is null");
        return 1;
      }
      delivery->storage.bool_value =
          *(const bool *)source ? (uint8_t)1u : (uint8_t)0u;
      return 1;
    case TRA_FFIC_TYPE_INT8:
      if (source == NULL) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, "Completion result is null");
        return 1;
      }
      delivery->storage.int8_value = *(const int8_t *)source;
      return 1;
    case TRA_FFIC_TYPE_UINT8:
      if (source == NULL) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, "Completion result is null");
        return 1;
      }
      delivery->storage.uint8_value = *(const uint8_t *)source;
      return 1;
    case TRA_FFIC_TYPE_INT16:
      if (source == NULL) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, "Completion result is null");
        return 1;
      }
      delivery->storage.int16_value = *(const int16_t *)source;
      return 1;
    case TRA_FFIC_TYPE_UINT16:
      if (source == NULL) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, "Completion result is null");
        return 1;
      }
      delivery->storage.uint16_value = *(const uint16_t *)source;
      return 1;
    case TRA_FFIC_TYPE_INT32:
      if (source == NULL) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, "Completion result is null");
        return 1;
      }
      delivery->storage.int32_value = *(const int32_t *)source;
      return 1;
    case TRA_FFIC_TYPE_UINT32:
      if (source == NULL) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, "Completion result is null");
        return 1;
      }
      delivery->storage.uint32_value = *(const uint32_t *)source;
      return 1;
    case TRA_FFIC_TYPE_INT64:
      if (source == NULL) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, "Completion result is null");
        return 1;
      }
      delivery->storage.int64_value = *(const int64_t *)source;
      return 1;
    case TRA_FFIC_TYPE_UINT64:
      if (source == NULL) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, "Completion result is null");
        return 1;
      }
      delivery->storage.uint64_value = *(const uint64_t *)source;
      return 1;
    case TRA_FFIC_TYPE_FLOAT:
      if (source == NULL) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, "Completion result is null");
        return 1;
      }
      delivery->storage.float_value = *(const float *)source;
      return 1;
    case TRA_FFIC_TYPE_DOUBLE:
      if (source == NULL) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, "Completion result is null");
        return 1;
      }
      delivery->storage.double_value = *(const double *)source;
      return 1;
    case TRA_FFIC_TYPE_BUFFER_VIEW:
      if (source == NULL) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, "Completion result is null");
        return 1;
      }
      delivery->storage.buffer_view_value =
          *(const tra_ffic_buffer_view *)source;
      if (!tra_ffic_buffer_view_is_valid(
              &delivery->storage.buffer_view_value)) {
        delivery->has_error = true;
        delivery->storage.buffer_view_value.data = NULL;
        delivery->storage.buffer_view_value.size = 0u;
        tra_ffic_error_set(&delivery->error, "Invalid buffer view");
      }
      return 1;
    case TRA_FFIC_TYPE_POINTER:
      if (source == NULL) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, "Completion result is null");
        return 1;
      }
      delivery->storage.pointer_value = *(void *const *)source;
      return 1;
    case TRA_FFIC_TYPE_STRING:
      if (source == NULL) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, "Completion result is null");
        return 1;
      }
      string_value = *(const char *const *)source;
      if (string_value == NULL) {
        delivery->storage.string_value = NULL;
        return 1;
      }
      delivery->string_storage = tra_ffic_string_duplicate(string_value);
      if (delivery->string_storage == NULL) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, "Out of memory copying string");
        return 1;
      }
      delivery->storage.string_value = delivery->string_storage;
      return 1;
    case TRA_FFIC_TYPE_FUNCTION:
      if (source == NULL) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, "Completion result is null");
        return 1;
      }
      function_value = *(tra_ffic_native_function const *)source;
      if (function_value == NULL) {
        delivery->storage.function_value = NULL;
        return 1;
      }
      if (!tra_ffic_store_function_for_expected(
              entry->owner_side, &entry->completion_return_type,
              function_value, &delivery->storage, &function_entry, false,
              &local_error)) {
        delivery->has_error = true;
        tra_ffic_error_set(&delivery->error, local_error.message);
        return 1;
      }
      delivery->function_entry = function_entry;
      return 1;
  }
  delivery->has_error = true;
  tra_ffic_error_set(&delivery->error, "Completion type is unsupported");
  return 1;
}

static inline void tra_ffic_completion_delivery_destroy(
    tra_ffic_completion_delivery *delivery) {
  if (delivery == NULL) {
    return;
  }
  tra_ffic_entry_release_active(delivery->function_entry);
  tra_ffic_entry_release_active(delivery->completion_entry);
  free(delivery->string_storage);
  free(delivery);
}

static inline ffi_type *tra_ffic_completion_callback_value_ffi_type(
    const tra_ffic_type *type) {
  return tra_ffic_get_ffi_type(type, false);
}

static inline void tra_ffic_completion_delivery_task(void *task_data) {
  tra_ffic_completion_delivery *delivery =
      (tra_ffic_completion_delivery *)task_data;
  tra_ffic_registry_entry *entry = NULL;
  ffi_cif cif;
  ffi_type *arg_types[3];
  void *ffi_values[3];
  void *user_data_value = NULL;
  const tra_ffic_error *error_value = NULL;
  uint32_t arg_count = 0u;
  uint32_t error_index = 0u;
  ffi_status status = FFI_OK;
  if (delivery == NULL || delivery->completion_entry == NULL) {
    tra_ffic_completion_delivery_destroy(delivery);
    return;
  }
  entry = delivery->completion_entry;
  user_data_value = entry->user_data;
  error_value = delivery->has_error ? &delivery->error : NULL;
  arg_types[0] = &ffi_type_pointer;
  ffi_values[0] = &user_data_value;
  arg_count = 1u;
  if (entry->completion_return_type.kind != TRA_FFIC_TYPE_VOID) {
    void *value_address = NULL;
    arg_types[arg_count] = tra_ffic_completion_callback_value_ffi_type(
        &entry->completion_return_type);
    if (arg_types[arg_count] == NULL) {
      tra_ffic_completion_delivery_destroy(delivery);
      return;
    }
    value_address = tra_ffic_arg_storage_value_address(
        entry->completion_return_type.kind, &delivery->storage);
    if (value_address == NULL) {
      tra_ffic_completion_delivery_destroy(delivery);
      return;
    }
    ffi_values[arg_count] = value_address;
    arg_count += 1u;
  }
  error_index = arg_count;
  arg_types[error_index] = &ffi_type_pointer;
  ffi_values[error_index] = &error_value;
  arg_count += 1u;
  status = ffi_prep_cif(&cif, FFI_DEFAULT_ABI, arg_count, &ffi_type_void,
                        arg_types);
  if (status == FFI_OK && entry->callback != NULL) {
    ffi_call(&cif, FFI_FN(entry->callback), NULL, ffi_values);
  }
  tra_ffic_completion_delivery_destroy(delivery);
}

static inline void tra_ffic_completion_function_trampoline(
    ffi_cif *cif,
    void *return_value,
    void **args,
    void *user_data) {
  tra_ffic_registry_entry *entry =
      (tra_ffic_registry_entry *)user_data;
  tra_ffic_registry_entry *active_entry = NULL;
  tra_ffic_completion_delivery *delivery = NULL;
  const void *result = NULL;
  const char *error_message = NULL;
  bool should_claim = false;
  (void)cif;
  (void)return_value;
  if (entry == NULL || args == NULL || !entry->is_completion_function) {
    return;
  }
  active_entry = tra_ffic_entry_add_active_entry(entry, NULL);
  if (active_entry == NULL) {
    return;
  }
  tra_ffic_mutex_lock(&entry->owner_side->mutex);
  if (!entry->completion_claimed && !entry->destroy_scheduled) {
    entry->completion_claimed = true;
    should_claim = true;
  }
  tra_ffic_mutex_unlock(&entry->owner_side->mutex);
  if (!should_claim) {
    tra_ffic_entry_release_active(active_entry);
    return;
  }
  result = *(const void **)args[0];
  error_message = *(const char **)args[1];
  delivery =
      (tra_ffic_completion_delivery *)calloc(1u, sizeof(*delivery));
  if (delivery == NULL) {
    tra_ffic_entry_release_active(active_entry);
    return;
  }
  delivery->completion_entry = active_entry;
  if (!tra_ffic_completion_copy_value(entry, result, error_message,
                                      delivery)) {
    tra_ffic_completion_delivery_destroy(delivery);
    return;
  }
  if (entry->owner_side->schedule == NULL ||
      !entry->owner_side->schedule(entry->owner_side->schedule_data,
                                   tra_ffic_completion_delivery_task,
                                   delivery)) {
    tra_ffic_completion_delivery_task(delivery);
  }
}

/**
 * Creates a completion function pointer owned by side.
 *
 * callback has ABI void(void *user_data, result, const tra_ffic_error *error)
 * for non-void result types, or void(void *user_data,
 * const tra_ffic_error *error) for void results.
 */
static inline int tra_ffic_side_create_completion_function_impl(
    tra_ffic_side *side,
    const tra_ffic_type *return_type,
    tra_ffic_completion_callback callback,
    tra_ffic_native_function *out_function,
    void *user_data,
    tra_ffic_error *error) {
  tra_ffic_registry_entry *entry = NULL;
  ffi_status status = FFI_OK;
  tra_ffic_error_clear(error);
  if (side == NULL || !side->initialized || return_type == NULL ||
      callback == NULL || out_function == NULL) {
    tra_ffic_error_set(error, "Create completion input is null");
    return 0;
  }
  *out_function = NULL;
  if (!tra_ffic_type_validate(return_type, true, 0u, error)) {
    return 0;
  }
  entry = (tra_ffic_registry_entry *)calloc(1u, sizeof(*entry));
  if (entry == NULL) {
    tra_ffic_error_set(error, "Out of memory creating completion");
    return 0;
  }
  entry->owner_side = side;
  entry->callback = (tra_ffic_user_function)callback;
  entry->user_data = user_data;
  entry->ref_count = 1u;
  entry->is_completion_function = true;
  entry->has_completion_return_type = true;
  if (!tra_ffic_type_clone(return_type, &entry->completion_return_type, 0u,
                           error)) {
    free(entry);
    return 0;
  }
  entry->ffi_arg_count = 2u;
  entry->ffi_arg_types =
      (ffi_type **)calloc(entry->ffi_arg_count, sizeof(*entry->ffi_arg_types));
  if (entry->ffi_arg_types == NULL) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error, "Out of memory creating completion ffi table");
    return 0;
  }
  entry->ffi_arg_types[0] = &ffi_type_pointer;
  entry->ffi_arg_types[1] = &ffi_type_pointer;
  status = ffi_prep_cif(&entry->cif, FFI_DEFAULT_ABI, entry->ffi_arg_count,
                        &ffi_type_void, entry->ffi_arg_types);
  if (status != FFI_OK) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error, "ffi_prep_cif failed for completion function");
    return 0;
  }
  entry->closure = tra_ffic_closure_alloc_tracked(&entry->closure_code);
  if (entry->closure == NULL || entry->closure_code == NULL) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error, "ffi_closure_alloc failed for completion");
    return 0;
  }
  status = ffi_prep_closure_loc(entry->closure, &entry->cif,
                                tra_ffic_completion_function_trampoline,
                                entry, entry->closure_code);
  if (status != FFI_OK) {
    tra_ffic_registry_entry_destroy(entry);
    tra_ffic_error_set(error,
                       "ffi_prep_closure_loc failed for completion function");
    return 0;
  }
  entry->raw = tra_ffic_native_from_code(entry->closure_code);
  tra_ffic_side_add_entry(side, entry);
  *out_function = entry->raw;
  return 1;
}

/** Releases all closures currently owned by side. */
static inline void tra_ffic_side_destroy(tra_ffic_side *side) {
  tra_ffic_registry_entry *entries = NULL;
  tra_ffic_registry_entry *entry = NULL;
  if (side == NULL || !side->initialized) {
    return;
  }
  tra_ffic_mutex_lock(&side->mutex);
  entries = side->entries;
  side->entries = NULL;
  side->initialized = false;
  tra_ffic_mutex_unlock(&side->mutex);

  entry = entries;
  while (entry != NULL) {
    tra_ffic_registry_entry *next = entry->next;
    entry->next = NULL;
    entry->ref_count = 0u;
    if (entry->active_call_count == 0u) {
      tra_ffic_global_registry_remove(entry);
      tra_ffic_registry_entry_schedule_destroy(entry);
    }
    entry = next;
  }
  tra_ffic_mutex_destroy(&side->mutex);
}

static inline int tra_ffic_validate_and_store_arg(
    tra_ffic_side *caller_side,
    const tra_ffic_type *expected_type,
    const tra_ffic_value *source,
    tra_ffic_arg_storage *storage,
    tra_ffic_registry_entry **protected_entry,
    tra_ffic_error *error) {
  tra_ffic_error local_error;
  tra_ffic_error_clear(&local_error);
  *protected_entry = NULL;
  if (expected_type == NULL || source == NULL || storage == NULL) {
    tra_ffic_error_set(error, "Argument input is null");
    return 0;
  }
  if (source->kind != expected_type->kind) {
    tra_ffic_error_set(error, "Argument type mismatch");
    return 0;
  }
  switch (expected_type->kind) {
    case TRA_FFIC_TYPE_BOOL:
      storage->bool_value = source->as.bool_value ? (uint8_t)1u : (uint8_t)0u;
      return 1;
    case TRA_FFIC_TYPE_INT8:
      storage->int8_value = source->as.int8_value;
      return 1;
    case TRA_FFIC_TYPE_UINT8:
      storage->uint8_value = source->as.uint8_value;
      return 1;
    case TRA_FFIC_TYPE_INT16:
      storage->int16_value = source->as.int16_value;
      return 1;
    case TRA_FFIC_TYPE_UINT16:
      storage->uint16_value = source->as.uint16_value;
      return 1;
    case TRA_FFIC_TYPE_INT32:
      storage->int32_value = source->as.int32_value;
      return 1;
    case TRA_FFIC_TYPE_UINT32:
      storage->uint32_value = source->as.uint32_value;
      return 1;
    case TRA_FFIC_TYPE_INT64:
      storage->int64_value = source->as.int64_value;
      return 1;
    case TRA_FFIC_TYPE_UINT64:
      storage->uint64_value = source->as.uint64_value;
      return 1;
    case TRA_FFIC_TYPE_FLOAT:
      storage->float_value = source->as.float_value;
      return 1;
    case TRA_FFIC_TYPE_DOUBLE:
      storage->double_value = source->as.double_value;
      return 1;
    case TRA_FFIC_TYPE_BUFFER_VIEW:
      if (!tra_ffic_buffer_view_is_valid(&source->as.buffer_view_value)) {
        tra_ffic_error_set(error, "Invalid buffer view");
        return 0;
      }
      storage->buffer_view_value = source->as.buffer_view_value;
      return 1;
    case TRA_FFIC_TYPE_POINTER:
      storage->pointer_value = source->as.pointer_value;
      return 1;
    case TRA_FFIC_TYPE_STRING:
      storage->string_value = source->as.string_value;
      return 1;
    case TRA_FFIC_TYPE_FUNCTION:
      if (source->as.function_value == NULL) {
        storage->function_value = NULL;
        (void)caller_side;
        return 1;
      }
      if (!tra_ffic_store_function_for_expected(
              caller_side, expected_type, source->as.function_value,
              storage, protected_entry, false, &local_error)) {
        tra_ffic_error_set(error, local_error.message);
        return 0;
      }
      (void)caller_side;
      return 1;
    case TRA_FFIC_TYPE_VOID:
      tra_ffic_error_set(error, "Void argument is invalid");
      return 0;
  }
  tra_ffic_error_set(error, "Unsupported argument type");
  return 0;
}

static inline int tra_ffic_result_set_retval_from_storage(
    tra_ffic_result *result,
    const tra_ffic_type *return_type,
    const tra_ffic_arg_storage *storage) {
  if (result == NULL || return_type == NULL) {
    return 0;
  }
  result->success = true;
  result->error_message[0] = '\0';
  if (return_type->kind != TRA_FFIC_TYPE_VOID && storage == NULL) {
    return tra_ffic_result_set_error(result, "Return storage is null");
  }
  switch (return_type->kind) {
    case TRA_FFIC_TYPE_VOID:
      result->value = tra_ffic_value_void();
      return 1;
    case TRA_FFIC_TYPE_BOOL:
      result->value = tra_ffic_value_bool(storage->bool_value != 0u);
      return 1;
    case TRA_FFIC_TYPE_INT8:
      result->value = tra_ffic_value_int8(storage->int8_value);
      return 1;
    case TRA_FFIC_TYPE_UINT8:
      result->value = tra_ffic_value_uint8(storage->uint8_value);
      return 1;
    case TRA_FFIC_TYPE_INT16:
      result->value = tra_ffic_value_int16(storage->int16_value);
      return 1;
    case TRA_FFIC_TYPE_UINT16:
      result->value = tra_ffic_value_uint16(storage->uint16_value);
      return 1;
    case TRA_FFIC_TYPE_INT32:
      result->value = tra_ffic_value_int32(storage->int32_value);
      return 1;
    case TRA_FFIC_TYPE_UINT32:
      result->value = tra_ffic_value_uint32(storage->uint32_value);
      return 1;
    case TRA_FFIC_TYPE_INT64:
      result->value = tra_ffic_value_int64(storage->int64_value);
      return 1;
    case TRA_FFIC_TYPE_UINT64:
      result->value = tra_ffic_value_uint64(storage->uint64_value);
      return 1;
    case TRA_FFIC_TYPE_FLOAT:
      result->value = tra_ffic_value_float(storage->float_value);
      return 1;
    case TRA_FFIC_TYPE_DOUBLE:
      result->value = tra_ffic_value_double(storage->double_value);
      return 1;
    case TRA_FFIC_TYPE_BUFFER_VIEW:
      if (!tra_ffic_buffer_view_is_valid(&storage->buffer_view_value)) {
        return tra_ffic_result_set_error(result, "Invalid buffer view");
      }
      result->value = tra_ffic_value_buffer_view(
          storage->buffer_view_value.data, storage->buffer_view_value.size);
      return 1;
    case TRA_FFIC_TYPE_POINTER:
      result->value = tra_ffic_value_pointer(storage->pointer_value);
      return 1;
    case TRA_FFIC_TYPE_STRING:
      result->value = tra_ffic_value_string(storage->string_value);
      return 1;
    case TRA_FFIC_TYPE_FUNCTION:
      result->value = tra_ffic_value_function(storage->function_value);
      return 1;
  }
  return tra_ffic_result_set_error(result, "Retval type is unsupported");
}

static inline int tra_ffic_call_with_retval_result(
    tra_ffic_side *caller_side,
    const tra_ffic_function_ref *target_ref,
    const tra_ffic_value *args,
    uint32_t arg_count,
    tra_ffic_result_callback result_callback,
    void *result_user_data,
    tra_ffic_error *error) {
  tra_ffic_registry_entry *target_entry = NULL;
  tra_ffic_registry_entry **protected_entries = NULL;
  uint32_t protected_count = 0u;
  tra_ffic_arg_storage *storages = NULL;
  tra_ffic_arg_storage return_storage;
  tra_ffic_registry_entry *result_function_entry = NULL;
  void **ffi_values = NULL;
  void **pointer_arg_list = NULL;
  void *pointer_arg_list_value = NULL;
  void *return_address = NULL;
  tra_ffic_result result;
  uint32_t index = 0u;
  uint32_t ffi_arg_count = 0u;
  int ok = 0;

  memset(&return_storage, 0, sizeof(return_storage));
  memset(&result, 0, sizeof(result));
  ffi_arg_count = tra_ffic_public_ffi_arg_count(target_ref->signature);
  target_entry = tra_ffic_entry_add_active(target_ref, error);
  if (target_entry == NULL) {
    return 0;
  }
  if (arg_count > 0u) {
    protected_entries =
        (tra_ffic_registry_entry **)calloc(arg_count,
                                           sizeof(*protected_entries));
    storages = (tra_ffic_arg_storage *)calloc(arg_count, sizeof(*storages));
    if (target_ref->signature->argument_passing ==
        TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST) {
      pointer_arg_list =
          (void **)calloc(arg_count, sizeof(*pointer_arg_list));
    }
    if (protected_entries == NULL || storages == NULL ||
        (target_ref->signature->argument_passing ==
             TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST &&
         pointer_arg_list == NULL)) {
      tra_ffic_error_set(error, "Out of memory preparing retval call");
      goto cleanup;
    }
  }
  if (ffi_arg_count > 0u) {
    ffi_values = (void **)calloc(ffi_arg_count, sizeof(*ffi_values));
    if (ffi_values == NULL) {
      tra_ffic_error_set(error, "Out of memory preparing retval call");
      goto cleanup;
    }
  }
  for (index = 0u; index < arg_count; ++index) {
    tra_ffic_registry_entry *protected_entry = NULL;
    void *value_address = NULL;
    if (!tra_ffic_validate_and_store_arg(
            caller_side, &target_ref->signature->arg_types[index],
            &args[index], &storages[index], &protected_entry, error)) {
      goto cleanup;
    }
    if (protected_entry != NULL) {
      protected_entries[protected_count] = protected_entry;
      protected_count += 1u;
    }
    value_address = tra_ffic_arg_storage_value_address(
        target_ref->signature->arg_types[index].kind, &storages[index]);
    if (value_address == NULL) {
      tra_ffic_error_set(error, "Unsupported argument type");
      goto cleanup;
    }
    if (target_ref->signature->argument_passing ==
        TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST) {
      pointer_arg_list[index] = value_address;
    } else {
      ffi_values[index] = value_address;
    }
  }
  if (target_ref->signature->argument_passing ==
      TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST) {
    pointer_arg_list_value = pointer_arg_list;
    ffi_values[0] = &pointer_arg_list_value;
  }
  if (target_ref->signature->return_type->kind != TRA_FFIC_TYPE_VOID) {
    return_address = tra_ffic_arg_storage_value_address(
        target_ref->signature->return_type->kind, &return_storage);
    if (return_address == NULL) {
      tra_ffic_error_set(error, "Unsupported return type");
      goto cleanup;
    }
  }
  ffi_call(&target_entry->cif, FFI_FN(target_ref->raw), return_address,
           ffi_values);
  if (target_ref->signature->return_type->kind == TRA_FFIC_TYPE_FUNCTION &&
      return_storage.function_value != NULL) {
    tra_ffic_arg_storage adapted_storage;
    memset(&adapted_storage, 0, sizeof(adapted_storage));
    if (!tra_ffic_store_function_for_expected(
            caller_side, target_ref->signature->return_type,
            return_storage.function_value, &adapted_storage,
            &result_function_entry, false, error)) {
      goto cleanup;
    }
    return_storage.function_value = adapted_storage.function_value;
  }
  (void)tra_ffic_result_set_retval_from_storage(
      &result, target_ref->signature->return_type, &return_storage);
  result_callback(result_user_data, &result);
  ok = 1;

cleanup:
  for (index = 0u; index < protected_count; ++index) {
    tra_ffic_entry_release_active(protected_entries[index]);
  }
  free(protected_entries);
  free(storages);
  free(pointer_arg_list);
  free(ffi_values);
  tra_ffic_entry_release_active(result_function_entry);
  tra_ffic_entry_release_active(target_entry);
  return ok;
}

/** Legacy internal helper for structured libffi calls. */
static inline int tra_ffic_call_with_result(
    tra_ffic_side *caller_side,
    const tra_ffic_function_ref *target_ref,
    const tra_ffic_value *args,
    uint32_t arg_count,
    tra_ffic_result_callback result_callback,
    void *result_user_data,
    tra_ffic_error *error) {
  tra_ffic_registry_entry *target_entry = NULL;
  tra_ffic_registry_entry **protected_entries = NULL;
  uint32_t protected_count = 0u;
  tra_ffic_arg_storage *storages = NULL;
  void **ffi_values = NULL;
  void **pointer_arg_list = NULL;
  void *pointer_arg_list_value = NULL;
  tra_ffic_pending_call *pending = NULL;
  uint32_t index = 0u;
  uint32_t ffi_arg_count = 0u;
  int ok = 0;
  tra_ffic_error_clear(error);
  if (caller_side == NULL || !caller_side->initialized || target_ref == NULL ||
      target_ref->signature == NULL || result_callback == NULL) {
    tra_ffic_error_set(error, "Call input is null");
    return 0;
  }
  if (target_ref->signature->arg_count != arg_count) {
    tra_ffic_error_set(error, "Invalid argument count");
    return 0;
  }
  if (arg_count > 0u && args == NULL) {
    tra_ffic_error_set(error, "Argument table is null");
    return 0;
  }
  if (target_ref->signature->abi == TRA_FFIC_SIGNATURE_ABI_RETVAL) {
    return tra_ffic_call_with_retval_result(
        caller_side, target_ref, args, arg_count, result_callback,
        result_user_data, error);
  }
  target_entry = tra_ffic_entry_add_active(target_ref, error);
  if (target_entry == NULL) {
    return 0;
  }
  ffi_arg_count = tra_ffic_public_ffi_arg_count(target_ref->signature);
  if (arg_count > 0u) {
    protected_entries =
        (tra_ffic_registry_entry **)calloc(arg_count,
                                              sizeof(*protected_entries));
    if (target_ref->signature->argument_passing ==
        TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST) {
      pointer_arg_list =
          (void **)calloc(arg_count, sizeof(*pointer_arg_list));
    }
  }
  storages =
      (tra_ffic_arg_storage *)calloc(arg_count + 1u, sizeof(*storages));
  ffi_values = (void **)calloc(ffi_arg_count, sizeof(*ffi_values));
  if ((arg_count > 0u && protected_entries == NULL) || storages == NULL ||
      ffi_values == NULL ||
      (target_ref->signature->argument_passing ==
           TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST &&
       arg_count > 0u && pointer_arg_list == NULL)) {
    tra_ffic_error_set(error, "Out of memory preparing call");
    goto cleanup;
  }
  for (index = 0u; index < arg_count; ++index) {
    tra_ffic_registry_entry *protected_entry = NULL;
    if (!tra_ffic_validate_and_store_arg(
            caller_side, &target_ref->signature->arg_types[index],
            &args[index], &storages[index + 1u], &protected_entry, error)) {
      goto cleanup;
    }
    if (protected_entry != NULL) {
      protected_entries[protected_count] = protected_entry;
      protected_count += 1u;
    }
  }
  pending = tra_ffic_pending_call_create(
      caller_side, target_ref->signature->return_type, protected_entries,
      protected_count, result_callback, result_user_data, error);
  if (pending == NULL) {
    goto cleanup;
  }
  protected_entries = NULL;
  protected_count = 0u;
  storages[0].completion_value = pending->completion;
  ffi_values[0] = &storages[0].completion_value;
  for (index = 0u; index < arg_count; ++index) {
    void *value_address = tra_ffic_arg_storage_value_address(
        target_ref->signature->arg_types[index].kind, &storages[index + 1u]);
    if (value_address == NULL) {
      tra_ffic_error_set(error, "Unsupported argument type");
      goto cleanup;
    }
    if (target_ref->signature->argument_passing ==
        TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST) {
      pointer_arg_list[index] = value_address;
    } else {
      ffi_values[index + 1u] = value_address;
    }
  }
  if (target_ref->signature->argument_passing ==
      TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST) {
    pointer_arg_list_value = pointer_arg_list;
    ffi_values[1] = &pointer_arg_list_value;
  }
  ffi_call(&target_entry->cif, FFI_FN(target_ref->raw), NULL, ffi_values);
  tra_ffic_mutex_lock(&pending->mutex);
  pending->native_returned = true;
  tra_ffic_mutex_unlock(&pending->mutex);
  tra_ffic_pending_try_finish(pending);
  pending = NULL;
  ok = 1;

cleanup:
  if (pending != NULL) {
    tra_ffic_pending_call_destroy(pending);
  }
  for (index = 0u; index < protected_count; ++index) {
    tra_ffic_entry_release_active(protected_entries[index]);
  }
  free(protected_entries);
  free(storages);
  free(pointer_arg_list);
  free(ffi_values);
  tra_ffic_entry_release_active(target_entry);
  return ok;
}

typedef struct tra_ffic_adapter_completion_state {
  tra_ffic_side *owner_side;
  tra_ffic_completion completion;
  tra_ffic_type return_type;
} tra_ffic_adapter_completion_state;

static inline void tra_ffic_adapter_completion_state_destroy(
    tra_ffic_adapter_completion_state *state) {
  if (state == NULL) {
    return;
  }
  tra_ffic_type_destroy(&state->return_type);
  free(state);
}

static inline void tra_ffic_adapter_complete_from_result(
    tra_ffic_side *owner_side,
    tra_ffic_completion completion,
    const tra_ffic_type *return_type,
    const tra_ffic_result *result) {
  tra_ffic_arg_storage storage;
  tra_ffic_registry_entry *protected_entry = NULL;
  tra_ffic_error error;
  void *value_address = NULL;
  memset(&storage, 0, sizeof(storage));
  tra_ffic_error_clear(&error);
  if (completion == NULL || return_type == NULL) {
    return;
  }
  if (result == NULL) {
    completion(NULL, "Adapter result is null");
    return;
  }
  if (!result->success) {
    completion(NULL, result->error_message);
    return;
  }
  if (return_type->kind == TRA_FFIC_TYPE_VOID) {
    completion(NULL, NULL);
    return;
  }
  if (!tra_ffic_store_value_for_type(owner_side, return_type, &result->value,
                                     &storage, &protected_entry, false,
                                     &error)) {
    completion(NULL, error.message);
    return;
  }
  value_address =
      tra_ffic_arg_storage_value_address(return_type->kind, &storage);
  completion(value_address, NULL);
  tra_ffic_entry_release_active(protected_entry);
}

static inline void tra_ffic_adapter_completion_callback(
    void *user_data,
    const tra_ffic_result *result) {
  tra_ffic_adapter_completion_state *state =
      (tra_ffic_adapter_completion_state *)user_data;
  if (state == NULL) {
    return;
  }
  tra_ffic_adapter_complete_from_result(
      state->owner_side, state->completion, &state->return_type, result);
  tra_ffic_adapter_completion_state_destroy(state);
}

typedef struct tra_ffic_adapter_retval_capture {
  tra_ffic_result result;
  bool called;
} tra_ffic_adapter_retval_capture;

static inline void tra_ffic_adapter_retval_callback(
    void *user_data,
    const tra_ffic_result *result) {
  tra_ffic_adapter_retval_capture *capture =
      (tra_ffic_adapter_retval_capture *)user_data;
  if (capture == NULL) {
    return;
  }
  capture->called = true;
  if (result != NULL) {
    capture->result = *result;
  } else {
    (void)tra_ffic_result_set_error(&capture->result,
                                    "Adapter result is null");
  }
}

static inline int tra_ffic_adapter_decode_args(
    tra_ffic_registry_entry *entry,
    void **args,
    tra_ffic_completion *completion,
    tra_ffic_value **out_values,
    tra_ffic_registry_entry ***out_protected_entries,
    uint32_t *out_protected_count,
    tra_ffic_error *error) {
  tra_ffic_arg_storage *storages = NULL;
  tra_ffic_value *values = NULL;
  tra_ffic_registry_entry **protected_entries = NULL;
  void *const *public_arg_list = NULL;
  uint32_t index = 0u;
  uint32_t protected_count = 0u;
  const bool uses_completion_abi =
      tra_ffic_signature_uses_completion(entry->signature);
  const uint32_t native_arg_offset = uses_completion_abi ? 1u : 0u;
  if (completion != NULL) {
    *completion = NULL;
  }
  *out_values = NULL;
  *out_protected_entries = NULL;
  *out_protected_count = 0u;
  if (uses_completion_abi && completion != NULL) {
    *completion = *(tra_ffic_completion *)args[0];
  }
  if (entry->signature->argument_passing ==
      TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST) {
    const uint32_t pointer_arg_index = uses_completion_abi ? 1u : 0u;
    public_arg_list = *(void *const **)args[pointer_arg_index];
    if (entry->signature->arg_count > 0u && public_arg_list == NULL) {
      tra_ffic_error_set(error, "Pointer-list argument table is null");
      return 0;
    }
  }
  if (entry->signature->arg_count == 0u) {
    return 1;
  }
  storages = (tra_ffic_arg_storage *)calloc(entry->signature->arg_count,
                                            sizeof(*storages));
  values =
      (tra_ffic_value *)calloc(entry->signature->arg_count, sizeof(*values));
  protected_entries = (tra_ffic_registry_entry **)calloc(
      entry->signature->arg_count, sizeof(*protected_entries));
  if (storages == NULL || values == NULL || protected_entries == NULL) {
    free(storages);
    free(values);
    free(protected_entries);
    tra_ffic_error_set(error, "Out of memory decoding adapter arguments");
    return 0;
  }
  for (index = 0u; index < entry->signature->arg_count; ++index) {
    const tra_ffic_type *type = &entry->signature->arg_types[index];
    void *raw_arg =
        entry->signature->argument_passing ==
                TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST
            ? public_arg_list[index]
            : args[index + native_arg_offset];
    tra_ffic_registry_entry *protected_entry = NULL;
    if (!tra_ffic_decode_raw_arg_to_storage(
            entry->owner_side, type, raw_arg, &storages[index],
            &values[index], &protected_entry, error)) {
      uint32_t release_index = 0u;
      for (release_index = 0u; release_index < protected_count;
           ++release_index) {
        tra_ffic_entry_release_active(protected_entries[release_index]);
      }
      free(storages);
      free(values);
      free(protected_entries);
      return 0;
    }
    if (protected_entry != NULL) {
      protected_entries[protected_count] = protected_entry;
      protected_count += 1u;
    }
  }
  free(storages);
  *out_values = values;
  *out_protected_entries = protected_entries;
  *out_protected_count = protected_count;
  return 1;
}

static inline void tra_ffic_function_adapter_trampoline(
    ffi_cif *cif,
    void *return_value,
    void **args,
    void *user_data) {
  tra_ffic_registry_entry *entry =
      (tra_ffic_registry_entry *)user_data;
  tra_ffic_function_adapter_state *state = NULL;
  tra_ffic_registry_entry *active_entry = NULL;
  tra_ffic_registry_entry **protected_entries = NULL;
  uint32_t protected_count = 0u;
  uint32_t index = 0u;
  tra_ffic_value *values = NULL;
  tra_ffic_completion completion = NULL;
  tra_ffic_error error;
  const bool uses_completion_abi =
      entry != NULL && tra_ffic_signature_uses_completion(entry->signature);
  (void)cif;
  tra_ffic_error_clear(&error);
  if (entry == NULL || entry->signature == NULL || args == NULL) {
    return;
  }
  state = (tra_ffic_function_adapter_state *)entry->user_data;
  if (state == NULL || state->source_ref.raw == NULL) {
    return;
  }
  if (!uses_completion_abi) {
    tra_ffic_store_zero_return(entry->signature->return_type, return_value);
  }
  active_entry = tra_ffic_entry_add_active_entry(entry, NULL);
  if (active_entry == NULL) {
    return;
  }
  if (!tra_ffic_adapter_decode_args(entry, args, &completion, &values,
                                    &protected_entries, &protected_count,
                                    &error)) {
    if (completion != NULL) {
      completion(NULL, error.message);
    }
    goto cleanup;
  }
  if (uses_completion_abi) {
    tra_ffic_adapter_completion_state *completion_state =
        (tra_ffic_adapter_completion_state *)calloc(1u,
                                                    sizeof(*completion_state));
    if (completion_state == NULL ||
        !tra_ffic_type_clone(entry->signature->return_type,
                             &completion_state->return_type, 0u, &error)) {
      free(completion_state);
      if (completion != NULL) {
        completion(NULL, "Out of memory creating adapter completion");
      }
      goto cleanup;
    }
    completion_state->owner_side = entry->owner_side;
    completion_state->completion = completion;
    if (!tra_ffic_call_with_result(
            entry->owner_side, &state->source_ref, values,
            entry->signature->arg_count,
            tra_ffic_adapter_completion_callback, completion_state, &error)) {
      tra_ffic_adapter_completion_state_destroy(completion_state);
      if (completion != NULL) {
        completion(NULL, error.message);
      }
      goto cleanup;
    }
  } else {
    tra_ffic_adapter_retval_capture capture;
    memset(&capture, 0, sizeof(capture));
    (void)tra_ffic_result_set_error(&capture.result,
                                    "Adapter result was not delivered");
    if (!tra_ffic_call_with_result(
            entry->owner_side, &state->source_ref, values,
            entry->signature->arg_count, tra_ffic_adapter_retval_callback,
            &capture, &error)) {
      goto cleanup;
    }
    if (capture.called && capture.result.success) {
      tra_ffic_arg_storage storage;
      tra_ffic_registry_entry *result_function_entry = NULL;
      memset(&storage, 0, sizeof(storage));
      if (tra_ffic_store_value_for_type(
              entry->owner_side, entry->signature->return_type,
              &capture.result.value, &storage, &result_function_entry, true,
              &error)) {
        (void)tra_ffic_copy_arg_storage_to_address(
            entry->signature->return_type, &storage, return_value);
        tra_ffic_entry_release_active(result_function_entry);
      }
    }
  }

cleanup:
  for (index = 0u; index < protected_count; ++index) {
    tra_ffic_entry_release_active(protected_entries[index]);
  }
  free(protected_entries);
  free(values);
  tra_ffic_entry_release_active(active_entry);
}

typedef struct tra_ffic_success_callback_context {
  tra_ffic_success_callback callback;
  void *user_data;
} tra_ffic_success_callback_context;

static inline int tra_ffic_invoke_success_callback(
    tra_ffic_success_callback callback,
    void *user_data,
    const tra_ffic_result *result) {
  ffi_cif cif;
  ffi_type *arg_types[2];
  void *ffi_values[2];
  tra_ffic_arg_storage storages[2];
  ffi_status status = FFI_OK;
  uint32_t arg_count = 1u;
  if (callback == NULL || result == NULL || !result->success) {
    return 0;
  }
  memset(storages, 0, sizeof(storages));
  storages[0].pointer_value = user_data;
  arg_types[0] = &ffi_type_pointer;
  ffi_values[0] = &storages[0].pointer_value;
  switch (result->value.kind) {
    case TRA_FFIC_TYPE_VOID:
      break;
    case TRA_FFIC_TYPE_BOOL:
      storages[1].bool_value =
          result->value.as.bool_value ? (uint8_t)1u : (uint8_t)0u;
      arg_types[1] = &ffi_type_uint8;
      ffi_values[1] = &storages[1].bool_value;
      arg_count = 2u;
      break;
    case TRA_FFIC_TYPE_INT8:
      storages[1].int8_value = result->value.as.int8_value;
      arg_types[1] = &ffi_type_sint8;
      ffi_values[1] = &storages[1].int8_value;
      arg_count = 2u;
      break;
    case TRA_FFIC_TYPE_UINT8:
      storages[1].uint8_value = result->value.as.uint8_value;
      arg_types[1] = &ffi_type_uint8;
      ffi_values[1] = &storages[1].uint8_value;
      arg_count = 2u;
      break;
    case TRA_FFIC_TYPE_INT16:
      storages[1].int16_value = result->value.as.int16_value;
      arg_types[1] = &ffi_type_sint16;
      ffi_values[1] = &storages[1].int16_value;
      arg_count = 2u;
      break;
    case TRA_FFIC_TYPE_UINT16:
      storages[1].uint16_value = result->value.as.uint16_value;
      arg_types[1] = &ffi_type_uint16;
      ffi_values[1] = &storages[1].uint16_value;
      arg_count = 2u;
      break;
    case TRA_FFIC_TYPE_INT32:
      storages[1].int32_value = result->value.as.int32_value;
      arg_types[1] = &ffi_type_sint32;
      ffi_values[1] = &storages[1].int32_value;
      arg_count = 2u;
      break;
    case TRA_FFIC_TYPE_UINT32:
      storages[1].uint32_value = result->value.as.uint32_value;
      arg_types[1] = &ffi_type_uint32;
      ffi_values[1] = &storages[1].uint32_value;
      arg_count = 2u;
      break;
    case TRA_FFIC_TYPE_INT64:
      storages[1].int64_value = result->value.as.int64_value;
      arg_types[1] = &ffi_type_sint64;
      ffi_values[1] = &storages[1].int64_value;
      arg_count = 2u;
      break;
    case TRA_FFIC_TYPE_UINT64:
      storages[1].uint64_value = result->value.as.uint64_value;
      arg_types[1] = &ffi_type_uint64;
      ffi_values[1] = &storages[1].uint64_value;
      arg_count = 2u;
      break;
    case TRA_FFIC_TYPE_FLOAT:
      storages[1].float_value = result->value.as.float_value;
      arg_types[1] = &ffi_type_float;
      ffi_values[1] = &storages[1].float_value;
      arg_count = 2u;
      break;
    case TRA_FFIC_TYPE_DOUBLE:
      storages[1].double_value = result->value.as.double_value;
      arg_types[1] = &ffi_type_double;
      ffi_values[1] = &storages[1].double_value;
      arg_count = 2u;
      break;
    case TRA_FFIC_TYPE_BUFFER_VIEW:
      storages[1].buffer_view_value = result->value.as.buffer_view_value;
      arg_types[1] = &tra_ffic_buffer_view_ffi_type;
      ffi_values[1] = &storages[1].buffer_view_value;
      arg_count = 2u;
      break;
    case TRA_FFIC_TYPE_POINTER:
      storages[1].pointer_value = result->value.as.pointer_value;
      arg_types[1] = &ffi_type_pointer;
      ffi_values[1] = &storages[1].pointer_value;
      arg_count = 2u;
      break;
    case TRA_FFIC_TYPE_STRING:
      storages[1].string_value = result->value.as.string_value;
      arg_types[1] = &ffi_type_pointer;
      ffi_values[1] = &storages[1].string_value;
      arg_count = 2u;
      break;
    case TRA_FFIC_TYPE_FUNCTION:
      storages[1].function_value = result->value.as.function_value;
      arg_types[1] = &ffi_type_pointer;
      ffi_values[1] = &storages[1].function_value;
      arg_count = 2u;
      break;
  }
  status =
      ffi_prep_cif(&cif, FFI_DEFAULT_ABI, arg_count, &ffi_type_void,
                   arg_types);
  if (status != FFI_OK) {
    return 0;
  }
  ffi_call(&cif, FFI_FN(callback), NULL, ffi_values);
  return 1;
}

static inline void tra_ffic_success_result_adapter(
    void *user_data,
    const tra_ffic_result *result) {
  tra_ffic_success_callback_context *context =
      (tra_ffic_success_callback_context *)user_data;
  if (context != NULL) {
    if (result != NULL && result->success) {
      (void)tra_ffic_invoke_success_callback(context->callback,
                                             context->user_data, result);
    }
    free(context);
  }
}

/** Legacy internal helper for success-only libffi calls. */
static inline int tra_ffic_call_impl(tra_ffic_side *caller_side,
                                     const tra_ffic_function_ref *target_ref,
                                     const tra_ffic_value *args,
                                     uint32_t arg_count,
                                     tra_ffic_success_callback result_callback,
                                     void *result_user_data,
                                     tra_ffic_error *error) {
  tra_ffic_success_callback_context *context = NULL;
  tra_ffic_error_clear(error);
  if (result_callback == NULL) {
    tra_ffic_error_set(error, "Call input is null");
    return 0;
  }
  context = (tra_ffic_success_callback_context *)calloc(1u, sizeof(*context));
  if (context == NULL) {
    tra_ffic_error_set(error, "Out of memory creating callback context");
    return 0;
  }
  context->callback = result_callback;
  context->user_data = result_user_data;
  if (!tra_ffic_call_with_result(caller_side, target_ref, args, arg_count,
                                 tra_ffic_success_result_adapter, context,
                                 error)) {
    free(context);
    return 0;
  }
  return 1;
}

/** Finds a registered function and fills a callable function reference. */
static inline int tra_ffic_function_ref_from_raw(
    tra_ffic_native_function raw,
    const tra_ffic_signature *signature,
    tra_ffic_function_ref *out_ref,
    tra_ffic_error *error) {
  tra_ffic_registry_entry *entry = NULL;
  tra_ffic_error_clear(error);
  if (raw == NULL || signature == NULL || out_ref == NULL) {
    tra_ffic_error_set(error, "Function reference input is null");
    return 0;
  }
  entry = tra_ffic_global_registry_find(raw, signature);
  if (entry == NULL || entry->owner_side == NULL) {
    tra_ffic_error_set(error, "Function reference is unknown");
    return 0;
  }
  out_ref->raw = raw;
  out_ref->owner_side = entry->owner_side;
  out_ref->signature = signature;
  return 1;
}

/** Creates a pure function from a typed callback function. */
#define tra_ffic_side_create_pure_function(side, signature, callback, out_ref, \
                                           error)                              \
  tra_ffic_side_create_pure_function_impl(                                    \
      (side), (signature), (tra_ffic_user_function)(callback),                 \
      (tra_ffic_native_function *)(out_ref), (error))

/** Creates a pure function from a pointer-list callback function. */
#define tra_ffic_side_create_pure_pointer_list_function(                       \
    side, signature, callback, out_ref, error)                                 \
  tra_ffic_side_create_pure_pointer_list_function_impl(                        \
      (side), (signature), (tra_ffic_user_function)(callback),                 \
      (tra_ffic_native_function *)(out_ref), (error))

/** Creates a stateful closure from a typed callback function. */
#define tra_ffic_side_create_closure(side, signature, callback, closure_state, \
                                     finalize_user_data, out_ref, error)       \
  tra_ffic_side_create_closure_impl(                                           \
      (side), (signature), (tra_ffic_user_function)(callback),                 \
      (closure_state), (finalize_user_data),                                   \
      (tra_ffic_native_function *)(out_ref), (error))

/** Creates a stateful closure from a pointer-list callback function. */
#define tra_ffic_side_create_pointer_list_closure(                             \
    side, signature, callback, closure_state, finalize_user_data, out_ref,     \
    error)                                                                     \
  tra_ffic_side_create_pointer_list_closure_impl(                              \
      (side), (signature), (tra_ffic_user_function)(callback),                 \
      (closure_state), (finalize_user_data),                                   \
      (tra_ffic_native_function *)(out_ref), (error))

/** Creates a raw closure from a decoded-argument callback function. */
#define tra_ffic_side_create_raw_closure(side, signature, callback,            \
                                         closure_state, finalize_user_data,    \
                                         out_ref, error)                       \
  tra_ffic_side_create_raw_closure_impl(                                       \
      (side), (signature), (tra_ffic_raw_closure_callback)(callback),          \
      (closure_state), (finalize_user_data),                                   \
      (tra_ffic_native_function *)(out_ref), (error))

/** Creates a completion function from a typed completion callback. */
#define tra_ffic_side_create_completion_function(side, return_type, callback,  \
                                                 out_ref, user_data, error)    \
  tra_ffic_side_create_completion_function_impl(                               \
      (side), (return_type), (tra_ffic_completion_callback)(callback),          \
      (tra_ffic_native_function *)(out_ref), (user_data), (error))

/** Retains a registered function pointer. Null is accepted as a no-op. */
#define tra_ffic_function_retain(function, error)                              \
  tra_ffic_function_retain_impl((tra_ffic_native_function)(function), (error))

/** Releases a registered function pointer. Null is accepted as a no-op. */
#define tra_ffic_function_release(function, error)                             \
  tra_ffic_function_release_impl((tra_ffic_native_function)(function), (error))

/** Legacy call wrapper. New code should call registered function pointers. */
#define tra_ffic_call(caller_side, target_ref, args, arg_count,                \
                      result_callback, result_user_data, error)                \
  tra_ffic_call_impl((caller_side), (target_ref), (args), (arg_count),         \
                     (tra_ffic_success_callback)(result_callback),             \
                     (result_user_data), (error))

#ifdef __cplusplus
}
#endif

#endif
