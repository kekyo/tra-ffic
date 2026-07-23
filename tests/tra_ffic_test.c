/* tra-ffic - Universal asynchronous full-duplex marshaling helper library.
 * Copyright (c) Kouji Matsui (@kekyo@mi.kekyo.net)
 * Under MIT.
 * https://github.com/kekyo/tra-ffic/
 */

#include "tra_ffic.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if TRA_FFIC_IN_POSIX
#include <pthread.h>
#elif TRA_FFIC_IN_WIN32
#include <windows.h>
#endif

typedef void (*void_func)(tra_ffic_completion completion);
typedef void (*bool_func)(tra_ffic_completion completion, bool value);
typedef void (*i8_func)(tra_ffic_completion completion, int8_t value);
typedef void (*u8_func)(tra_ffic_completion completion, uint8_t value);
typedef void (*i16_func)(tra_ffic_completion completion, int16_t value);
typedef void (*u16_func)(tra_ffic_completion completion, uint16_t value);
typedef void (*i32_func)(tra_ffic_completion completion, int32_t value);
typedef void (*u32_func)(tra_ffic_completion completion, uint32_t value);
typedef void (*i64_func)(tra_ffic_completion completion, int64_t value);
typedef void (*u64_func)(tra_ffic_completion completion, uint64_t value);
typedef void (*f32_func)(tra_ffic_completion completion, float value);
typedef void (*f64_func)(tra_ffic_completion completion, double value);
typedef void (*pointer_func)(tra_ffic_completion completion, void *value);
typedef void (*string_func)(tra_ffic_completion completion, const char *value);
typedef void (*buffer_view_func)(tra_ffic_completion completion,
                                 tra_ffic_buffer_view value);
typedef void (*add_func)(tra_ffic_completion completion,
                         int32_t a,
                         int32_t b);
typedef void (*args_add_func)(tra_ffic_completion completion,
                              const void *const *args);
typedef void (*args_i32_func)(tra_ffic_completion completion,
                              const void *const *args);
typedef void (*function_arg_func)(tra_ffic_completion completion,
                                  i32_func callback);
typedef void (*args_function_arg_func)(tra_ffic_completion completion,
                                       args_i32_func callback);
typedef void (*function_arg_arg_func)(tra_ffic_completion completion,
                                      function_arg_func callback);
typedef void (*function_factory_func)(tra_ffic_completion completion);
typedef void (*retval_void_func)(void);
typedef int32_t (*retval_i32_func)(int32_t value);
typedef int32_t (*retval_add_func)(int32_t a, int32_t b);
typedef int32_t (*retval_args_add_func)(const void *const *args);
typedef double (*retval_f64_func)(double value);
typedef const char *(*retval_string_factory_func)(void);
typedef tra_ffic_buffer_view (*retval_buffer_view_factory_func)(void);
typedef retval_i32_func (*retval_function_factory_func)(void);
typedef int32_t (*retval_function_arg_func)(retval_i32_func callback);

typedef struct basic_struct_inner {
  uint8_t flag;
  double ratio;
} basic_struct_inner;

typedef struct basic_struct_value {
  int32_t number;
  basic_struct_inner nested;
  uint64_t total;
} basic_struct_value;

typedef void (*basic_struct_func)(tra_ffic_completion completion,
                                  basic_struct_value value);
typedef basic_struct_value (*retval_basic_struct_func)(
    basic_struct_value value);

typedef enum test_drain_mode {
  TEST_DRAIN_INLINE,
  TEST_DRAIN_THREAD,
} test_drain_mode;

typedef int (*leak_case_function)(test_drain_mode drain_mode);

typedef struct test_context {
  tra_ffic_task_queue queue;
  tra_ffic_side side_a;
  tra_ffic_side side_b;
  test_drain_mode drain_mode;
} test_context;

typedef struct typed_capture {
  int count;
  bool bool_value;
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
  void *pointer_value;
  tra_ffic_buffer_view buffer_view_value;
  char string_value[128];
  const char *string_pointer;
  int string_was_null;
  i32_func function_value;
  tra_ffic_native_function native_function_value;
  int retain_function;
  int has_error;
  char error_message[TRA_FFIC_ERROR_MESSAGE_CAPACITY];
} typed_capture;

typedef struct basic_struct_capture {
  int count;
  int has_error;
  basic_struct_value value;
  char error_message[TRA_FFIC_ERROR_MESSAGE_CAPACITY];
} basic_struct_capture;

typedef struct finalize_counter {
  int count;
} finalize_counter;

typedef struct nested_callback_state {
  tra_ffic_side *caller_side;
  tra_ffic_task_queue *queue;
  test_drain_mode drain_mode;
} nested_callback_state;

typedef struct nested_function_arg_state {
  tra_ffic_side *caller_side;
  tra_ffic_task_queue *queue;
  test_drain_mode drain_mode;
  i32_func inner_function;
} nested_function_arg_state;

typedef enum three_level_terminal_case {
  THREE_LEVEL_I32_TO_F64,
  THREE_LEVEL_U8_TO_I16,
  THREE_LEVEL_F32_TO_BOOL,
  THREE_LEVEL_F64_TO_I32,
  THREE_LEVEL_BOOL_TO_U64,
  THREE_LEVEL_U16_TO_F32,
  THREE_LEVEL_I64_TO_U32,
  THREE_LEVEL_U32_TO_I8,
} three_level_terminal_case;

typedef struct three_level_signature_set {
  tra_ffic_type terminal_arg_types[1];
  tra_ffic_type arg_types[3][1];
  tra_ffic_type function_types[4];
  tra_ffic_signature signatures[4];
} three_level_signature_set;

typedef struct three_level_result_capture {
  int count;
  int has_error;
  tra_ffic_value value;
  char error_message[TRA_FFIC_ERROR_MESSAGE_CAPACITY];
} three_level_result_capture;

typedef struct three_level_case_state three_level_case_state;

typedef struct three_level_node_state {
  three_level_case_state *case_state;
  uint32_t index;
} three_level_node_state;

struct three_level_case_state {
  const char *name;
  const char *pattern;
  three_level_terminal_case terminal_case;
  tra_ffic_side *invoke_side;
  tra_ffic_side *callback_side;
  tra_ffic_task_queue *queue;
  test_drain_mode drain_mode;
  const tra_ffic_type *terminal_arg_type;
  const tra_ffic_type *terminal_return_type;
  tra_ffic_value terminal_arg_value;
  tra_ffic_value expected_result;
  three_level_signature_set signature_set;
  tra_ffic_function_ref refs[4];
  three_level_node_state nodes[3];
};

typedef struct drain_thread_input {
  tra_ffic_task_queue *queue;
} drain_thread_input;

typedef struct task_queue_notification_capture {
  tra_ffic_task_queue *last_queue;
  int notifications;
  int ran;
  int drain_on_notify;
} task_queue_notification_capture;

typedef struct async_state {
#if TRA_FFIC_IN_POSIX
  pthread_t thread;
#elif TRA_FFIC_IN_WIN32
  HANDLE thread;
#endif
  int thread_started;
} async_state;

typedef struct async_thread_input {
  tra_ffic_completion completion;
  int32_t value;
} async_thread_input;

typedef struct release_during_call_state {
  i32_func self;
  finalize_counter *finalizer;
} release_during_call_state;

static i32_func g_seen_function_argument = NULL;

#define TEST_TYPE(kind, signature) \
  { (kind), (signature), 0u, NULL }

static const tra_ffic_type k_type_void =
    TEST_TYPE(TRA_FFIC_TYPE_VOID, NULL);
static const tra_ffic_type k_type_bool =
    TEST_TYPE(TRA_FFIC_TYPE_BOOL, NULL);
static const tra_ffic_type k_type_i8 =
    TEST_TYPE(TRA_FFIC_TYPE_INT8, NULL);
static const tra_ffic_type k_type_u8 =
    TEST_TYPE(TRA_FFIC_TYPE_UINT8, NULL);
static const tra_ffic_type k_type_i16 =
    TEST_TYPE(TRA_FFIC_TYPE_INT16, NULL);
static const tra_ffic_type k_type_u16 =
    TEST_TYPE(TRA_FFIC_TYPE_UINT16, NULL);
static const tra_ffic_type k_type_i32 =
    TEST_TYPE(TRA_FFIC_TYPE_INT32, NULL);
static const tra_ffic_type k_type_u32 =
    TEST_TYPE(TRA_FFIC_TYPE_UINT32, NULL);
static const tra_ffic_type k_type_i64 =
    TEST_TYPE(TRA_FFIC_TYPE_INT64, NULL);
static const tra_ffic_type k_type_u64 =
    TEST_TYPE(TRA_FFIC_TYPE_UINT64, NULL);
static const tra_ffic_type k_type_f32 =
    TEST_TYPE(TRA_FFIC_TYPE_FLOAT, NULL);
static const tra_ffic_type k_type_f64 =
    TEST_TYPE(TRA_FFIC_TYPE_DOUBLE, NULL);
static const tra_ffic_type k_type_pointer =
    TEST_TYPE(TRA_FFIC_TYPE_POINTER, NULL);
static const tra_ffic_type k_type_string =
    TEST_TYPE(TRA_FFIC_TYPE_STRING, NULL);
static const tra_ffic_type k_type_buffer_view = {
    TRA_FFIC_TYPE_BUFFER_VIEW, NULL, 0u, NULL};

static const tra_ffic_type k_sig_one_bool_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_BOOL, NULL)};
static const tra_ffic_signature k_sig_echo_bool = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1, k_sig_one_bool_args, &k_type_bool, TRA_FFIC_ARGUMENT_PASSING_STACK};

static const tra_ffic_type k_sig_one_i8_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_INT8, NULL)};
static const tra_ffic_signature k_sig_echo_i8 = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1, k_sig_one_i8_args, &k_type_i8, TRA_FFIC_ARGUMENT_PASSING_STACK};

static const tra_ffic_type k_sig_one_u8_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_UINT8, NULL)};
static const tra_ffic_signature k_sig_echo_u8 = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1, k_sig_one_u8_args, &k_type_u8, TRA_FFIC_ARGUMENT_PASSING_STACK};

static const tra_ffic_type k_sig_one_i16_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_INT16, NULL)};
static const tra_ffic_signature k_sig_echo_i16 = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1, k_sig_one_i16_args, &k_type_i16, TRA_FFIC_ARGUMENT_PASSING_STACK};

static const tra_ffic_type k_sig_one_u16_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_UINT16, NULL)};
static const tra_ffic_signature k_sig_echo_u16 = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1, k_sig_one_u16_args, &k_type_u16, TRA_FFIC_ARGUMENT_PASSING_STACK};

static const tra_ffic_type k_sig_one_i32_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_INT32, NULL)};
static const tra_ffic_signature k_sig_echo_i32 = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1, k_sig_one_i32_args, &k_type_i32, TRA_FFIC_ARGUMENT_PASSING_STACK};

static const tra_ffic_type k_sig_two_i32_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_INT32, NULL),
    TEST_TYPE(TRA_FFIC_TYPE_INT32, NULL)};
static const tra_ffic_signature k_sig_add_i32 = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 2, k_sig_two_i32_args, &k_type_i32, TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_signature k_sig_args_add_i32 = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION,
    2,
    k_sig_two_i32_args,
    &k_type_i32,
    TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST};

static const tra_ffic_type k_sig_one_u32_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_UINT32, NULL)};
static const tra_ffic_signature k_sig_echo_u32 = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1, k_sig_one_u32_args, &k_type_u32, TRA_FFIC_ARGUMENT_PASSING_STACK};

static const tra_ffic_type k_sig_one_i64_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_INT64, NULL)};
static const tra_ffic_signature k_sig_echo_i64 = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1, k_sig_one_i64_args, &k_type_i64, TRA_FFIC_ARGUMENT_PASSING_STACK};

static const tra_ffic_type k_sig_one_u64_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_UINT64, NULL)};
static const tra_ffic_signature k_sig_echo_u64 = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1, k_sig_one_u64_args, &k_type_u64, TRA_FFIC_ARGUMENT_PASSING_STACK};

static const tra_ffic_type k_sig_one_f32_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_FLOAT, NULL)};
static const tra_ffic_signature k_sig_echo_f32 = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1, k_sig_one_f32_args, &k_type_f32, TRA_FFIC_ARGUMENT_PASSING_STACK};

static const tra_ffic_type k_sig_one_f64_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_DOUBLE, NULL)};
static const tra_ffic_signature k_sig_echo_f64 = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1, k_sig_one_f64_args, &k_type_f64, TRA_FFIC_ARGUMENT_PASSING_STACK};

static const tra_ffic_type k_sig_one_pointer_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_POINTER, NULL)};
static const tra_ffic_signature k_sig_echo_pointer = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1, k_sig_one_pointer_args,
    &k_type_pointer, TRA_FFIC_ARGUMENT_PASSING_STACK};

static const tra_ffic_type k_sig_one_string_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_STRING, NULL)};
static const tra_ffic_signature k_sig_echo_string = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1, k_sig_one_string_args,
    &k_type_string, TRA_FFIC_ARGUMENT_PASSING_STACK};

static const tra_ffic_type k_sig_one_buffer_view_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_BUFFER_VIEW, NULL)};
static const tra_ffic_signature k_sig_echo_buffer_view = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1, k_sig_one_buffer_view_args,
    &k_type_buffer_view, TRA_FFIC_ARGUMENT_PASSING_STACK};

static const tra_ffic_signature k_sig_void = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 0, NULL, &k_type_void, TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_signature k_sig_return_f32 = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 0, NULL, &k_type_f32, TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_signature k_sig_return_i32 = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 0, NULL, &k_type_i32, TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_signature k_sig_return_string = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 0, NULL, &k_type_string, TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_signature k_sig_return_f64 = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 0, NULL, &k_type_f64, TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_signature k_sig_return_buffer_view = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 0, NULL, &k_type_buffer_view, TRA_FFIC_ARGUMENT_PASSING_STACK};

static const tra_ffic_type k_type_i32_function = {
    TRA_FFIC_TYPE_FUNCTION, &k_sig_echo_i32, 0u, NULL};
static const tra_ffic_signature k_sig_args_echo_i32 = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION,
    1,
    k_sig_one_i32_args,
    &k_type_i32,
    TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST};
static const tra_ffic_type k_type_args_i32_function = {
    TRA_FFIC_TYPE_FUNCTION, &k_sig_args_echo_i32, 0u, NULL};
static const tra_ffic_type k_sig_function_arg_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_FUNCTION, &k_sig_echo_i32)};
static const tra_ffic_signature k_sig_function_arg = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1, k_sig_function_arg_args,
    &k_type_i32, TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_type k_sig_args_function_arg_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_FUNCTION, &k_sig_args_echo_i32)};
static const tra_ffic_signature k_sig_args_function_arg = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION,
    1,
    k_sig_args_function_arg_args,
    &k_type_i32,
    TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_type k_sig_function_arg_arg_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_FUNCTION, &k_sig_function_arg)};
static const tra_ffic_signature k_sig_function_arg_arg = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1, k_sig_function_arg_arg_args,
    &k_type_i32, TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_signature k_sig_return_function = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION, 0, NULL, &k_type_i32_function, TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_signature k_sig_return_args_function = {
    TRA_FFIC_SIGNATURE_ABI_COMPLETION,
    0,
    NULL,
    &k_type_args_i32_function,
    TRA_FFIC_ARGUMENT_PASSING_STACK};

static const tra_ffic_signature k_retval_sig_add_i32 = {
    TRA_FFIC_SIGNATURE_ABI_RETVAL, 2, k_sig_two_i32_args, &k_type_i32, TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_signature k_retval_sig_args_add_i32 = {
    TRA_FFIC_SIGNATURE_ABI_RETVAL,
    2,
    k_sig_two_i32_args,
    &k_type_i32,
    TRA_FFIC_ARGUMENT_PASSING_POINTER_LIST};
static const tra_ffic_signature k_retval_sig_echo_i32 = {
    TRA_FFIC_SIGNATURE_ABI_RETVAL, 1, k_sig_one_i32_args, &k_type_i32, TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_signature k_retval_sig_echo_f64 = {
    TRA_FFIC_SIGNATURE_ABI_RETVAL, 1, k_sig_one_f64_args, &k_type_f64, TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_signature k_retval_sig_void = {
    TRA_FFIC_SIGNATURE_ABI_RETVAL, 0, NULL, &k_type_void, TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_signature k_retval_sig_return_string = {
    TRA_FFIC_SIGNATURE_ABI_RETVAL, 0, NULL, &k_type_string, TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_signature k_retval_sig_return_buffer_view = {
    TRA_FFIC_SIGNATURE_ABI_RETVAL, 0, NULL, &k_type_buffer_view, TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_type k_type_retval_i32_function = {
    TRA_FFIC_TYPE_FUNCTION, &k_retval_sig_echo_i32, 0u, NULL};
static const tra_ffic_type k_retval_sig_function_arg_args[] = {
    TEST_TYPE(TRA_FFIC_TYPE_FUNCTION, &k_retval_sig_echo_i32)};
static const tra_ffic_signature k_retval_sig_function_arg = {
    TRA_FFIC_SIGNATURE_ABI_RETVAL, 1, k_retval_sig_function_arg_args,
    &k_type_i32, TRA_FFIC_ARGUMENT_PASSING_STACK};
static const tra_ffic_signature k_retval_sig_return_function = {
    TRA_FFIC_SIGNATURE_ABI_RETVAL, 0, NULL, &k_type_retval_i32_function, TRA_FFIC_ARGUMENT_PASSING_STACK};

#undef TEST_TYPE

static int expect_true(int condition, const char *message) {
  if (!condition) {
    fprintf(stderr, "%s\n", message);
    return 0;
  }
  return 1;
}

static int is_positive_infinity_float(float value) {
  return isinf(value) && value > 0.0f;
}

static int is_positive_infinity_double(double value) {
  return isinf(value) && value > 0.0;
}

static void capture_error(typed_capture *capture,
                          const tra_ffic_error *error) {
  capture->has_error = error != NULL;
  capture->error_message[0] = '\0';
  if (error != NULL) {
    (void)snprintf(capture->error_message, sizeof(capture->error_message),
                   "%s", error->message);
  }
}

static void capture_void_callback(void *user_data,
                                  const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture_error(capture, error);
}

static void capture_bool_callback(void *user_data,
                                  bool result,
                                  const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture->bool_value = result;
  capture_error(capture, error);
}

static void capture_i8_callback(void *user_data,
                                int8_t result,
                                const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture->int8_value = result;
  capture_error(capture, error);
}

static void capture_u8_callback(void *user_data,
                                uint8_t result,
                                const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture->uint8_value = result;
  capture_error(capture, error);
}

static void capture_i16_callback(void *user_data,
                                 int16_t result,
                                 const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture->int16_value = result;
  capture_error(capture, error);
}

static void capture_u16_callback(void *user_data,
                                 uint16_t result,
                                 const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture->uint16_value = result;
  capture_error(capture, error);
}

static void capture_i32_callback(void *user_data,
                                 int32_t result,
                                 const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture->int32_value = result;
  capture_error(capture, error);
}

static void capture_u32_callback(void *user_data,
                                 uint32_t result,
                                 const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture->uint32_value = result;
  capture_error(capture, error);
}

static void capture_i64_callback(void *user_data,
                                 int64_t result,
                                 const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture->int64_value = result;
  capture_error(capture, error);
}

static void capture_u64_callback(void *user_data,
                                 uint64_t result,
                                 const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture->uint64_value = result;
  capture_error(capture, error);
}

static void capture_f32_callback(void *user_data,
                                 float result,
                                 const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture->float_value = result;
  capture_error(capture, error);
}

static void capture_f64_callback(void *user_data,
                                 double result,
                                 const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture->double_value = result;
  capture_error(capture, error);
}

static void capture_pointer_callback(void *user_data,
                                     void *result,
                                     const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture->pointer_value = result;
  capture_error(capture, error);
}

static void capture_buffer_view_callback(
    void *user_data,
    tra_ffic_buffer_view result,
    const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture->buffer_view_value = result;
  capture_error(capture, error);
}

#define DEFINE_SUCCESS_CAPTURE(name, value_type, field_name) \
  static void name(void *user_data, value_type result) {      \
    typed_capture *capture = (typed_capture *)user_data;      \
    capture->count += 1;                                      \
    capture->field_name = result;                             \
  }

DEFINE_SUCCESS_CAPTURE(capture_i8_success_callback, int8_t, int8_value)
DEFINE_SUCCESS_CAPTURE(capture_u8_success_callback, uint8_t, uint8_value)
DEFINE_SUCCESS_CAPTURE(capture_i16_success_callback, int16_t, int16_value)
DEFINE_SUCCESS_CAPTURE(capture_u16_success_callback, uint16_t, uint16_value)
DEFINE_SUCCESS_CAPTURE(capture_i32_success_callback, int32_t, int32_value)
DEFINE_SUCCESS_CAPTURE(capture_i64_success_callback, int64_t, int64_value)
DEFINE_SUCCESS_CAPTURE(capture_u64_success_callback, uint64_t, uint64_value)
DEFINE_SUCCESS_CAPTURE(capture_f32_success_callback, float, float_value)
DEFINE_SUCCESS_CAPTURE(capture_f64_success_callback, double, double_value)
DEFINE_SUCCESS_CAPTURE(capture_pointer_success_callback, void *, pointer_value)
DEFINE_SUCCESS_CAPTURE(capture_buffer_view_success_callback,
                       tra_ffic_buffer_view, buffer_view_value)

#undef DEFINE_SUCCESS_CAPTURE

static void capture_string_callback(void *user_data,
                                    const char *result,
                                    const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture_error(capture, error);
  if (result != NULL) {
    (void)snprintf(capture->string_value, sizeof(capture->string_value),
                   "%s", result);
  } else {
    capture->string_was_null = 1;
  }
}

static void capture_string_success_callback(void *user_data,
                                            const char *result) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  if (result != NULL) {
    (void)snprintf(capture->string_value, sizeof(capture->string_value),
                   "%s", result);
  } else {
    capture->string_was_null = 1;
  }
}

static void capture_function_callback(void *user_data,
                                      i32_func result,
                                      const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture->function_value = result;
  capture->native_function_value = (tra_ffic_native_function)result;
  capture_error(capture, error);
  if (error == NULL && capture->retain_function &&
      !tra_ffic_function_retain(result, &(tra_ffic_error){0})) {
    capture->has_error = 1;
    (void)snprintf(capture->error_message, sizeof(capture->error_message),
                   "%s", "failed to retain function result");
  }
}

static void capture_args_function_callback(void *user_data,
                                           args_i32_func result,
                                           const tra_ffic_error *error) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture->native_function_value = (tra_ffic_native_function)result;
  capture_error(capture, error);
  if (error == NULL && capture->retain_function &&
      !tra_ffic_function_retain(result, &(tra_ffic_error){0})) {
    capture->has_error = 1;
    (void)snprintf(capture->error_message, sizeof(capture->error_message),
                   "%s", "failed to retain args function result");
  }
}

static void capture_result_callback(void *user_data,
                                    const tra_ffic_result *result) {
  typed_capture *capture = (typed_capture *)user_data;
  capture->count += 1;
  capture->has_error = 0;
  capture->error_message[0] = '\0';
  if (result == NULL) {
    capture->has_error = 1;
    (void)snprintf(capture->error_message, sizeof(capture->error_message),
                   "%s", "result is null");
    return;
  }
  if (!result->success) {
    capture->has_error = 1;
    (void)snprintf(capture->error_message, sizeof(capture->error_message),
                   "%s", result->error_message);
    return;
  }
  switch (result->value.kind) {
    case TRA_FFIC_TYPE_VOID:
      break;
    case TRA_FFIC_TYPE_BOOL:
      capture->bool_value = result->value.as.bool_value;
      break;
    case TRA_FFIC_TYPE_INT8:
      capture->int8_value = result->value.as.int8_value;
      break;
    case TRA_FFIC_TYPE_UINT8:
      capture->uint8_value = result->value.as.uint8_value;
      break;
    case TRA_FFIC_TYPE_INT16:
      capture->int16_value = result->value.as.int16_value;
      break;
    case TRA_FFIC_TYPE_UINT16:
      capture->uint16_value = result->value.as.uint16_value;
      break;
    case TRA_FFIC_TYPE_INT32:
      capture->int32_value = result->value.as.int32_value;
      break;
    case TRA_FFIC_TYPE_UINT32:
      capture->uint32_value = result->value.as.uint32_value;
      break;
    case TRA_FFIC_TYPE_INT64:
      capture->int64_value = result->value.as.int64_value;
      break;
    case TRA_FFIC_TYPE_UINT64:
      capture->uint64_value = result->value.as.uint64_value;
      break;
    case TRA_FFIC_TYPE_FLOAT:
      capture->float_value = result->value.as.float_value;
      break;
    case TRA_FFIC_TYPE_DOUBLE:
      capture->double_value = result->value.as.double_value;
      break;
    case TRA_FFIC_TYPE_POINTER:
      capture->pointer_value = result->value.as.pointer_value;
      break;
    case TRA_FFIC_TYPE_BUFFER_VIEW:
      capture->buffer_view_value = result->value.as.buffer_view_value;
      break;
    case TRA_FFIC_TYPE_STRING:
      capture->string_pointer = result->value.as.string_value;
      if (result->value.as.string_value != NULL) {
        (void)snprintf(capture->string_value, sizeof(capture->string_value),
                       "%s", result->value.as.string_value);
      } else {
        capture->string_was_null = 1;
      }
      break;
    case TRA_FFIC_TYPE_FUNCTION:
      capture->function_value = (i32_func)result->value.as.function_value;
      capture->native_function_value = result->value.as.function_value;
      break;
    case TRA_FFIC_TYPE_STRUCT:
      capture->pointer_value = (void *)result->value.as.struct_value;
      break;
  }
}

#if TRA_FFIC_IN_WIN32
static DWORD WINAPI drain_thread_main(LPVOID user_data) {
#else
static void *drain_thread_main(void *user_data) {
#endif
  drain_thread_input *input = (drain_thread_input *)user_data;
  tra_ffic_task_drain_finalization(input->queue);
#if TRA_FFIC_IN_WIN32
  return 0;
#else
  return NULL;
#endif
}

static int test_task_queue_drain(tra_ffic_task_queue *queue,
                                 test_drain_mode drain_mode) {
  drain_thread_input input;
  if (drain_mode == TEST_DRAIN_INLINE) {
    tra_ffic_task_drain_finalization(queue);
    return 1;
  }
  input.queue = queue;
#if TRA_FFIC_IN_WIN32
  HANDLE thread = CreateThread(NULL, 0, drain_thread_main, &input, 0, NULL);
  if (thread == NULL) {
    fprintf(stderr, "failed to start drain thread\n");
    return 0;
  }
  if (WaitForSingleObject(thread, INFINITE) == WAIT_FAILED) {
    (void)CloseHandle(thread);
    fprintf(stderr, "failed to join drain thread\n");
    return 0;
  }
  (void)CloseHandle(thread);
#else
  pthread_t thread;
  if (pthread_create(&thread, NULL, drain_thread_main, &input) != 0) {
    fprintf(stderr, "failed to start drain thread\n");
    return 0;
  }
  if (pthread_join(thread, NULL) != 0) {
    fprintf(stderr, "failed to join drain thread\n");
    return 0;
  }
#endif
  return 1;
}

static int test_context_drain(test_context *context) {
  return test_task_queue_drain(&context->queue, context->drain_mode);
}

static int test_context_init(test_context *context,
                             test_drain_mode drain_mode) {
  tra_ffic_error error;
  if (!tra_ffic_task_queue_init(&context->queue, NULL, NULL)) {
    fprintf(stderr, "failed to initialize task queue\n");
    return 0;
  }
  context->drain_mode = drain_mode;
  if (!tra_ffic_side_init_pair(&context->side_a, &context->side_b,
                               tra_ffic_task_queue_schedule_callback,
                               &context->queue, &error)) {
    fprintf(stderr, "%s\n", error.message);
    tra_ffic_task_queue_destroy(&context->queue);
    return 0;
  }
  return 1;
}

static int test_context_destroy(test_context *context) {
  int passed = 1;
  tra_ffic_side_destroy(&context->side_a);
  tra_ffic_side_destroy(&context->side_b);
  passed = test_context_drain(context) && passed;
  tra_ffic_task_queue_destroy(&context->queue);
  return passed;
}

static void task_queue_notification_callback(
    tra_ffic_task_queue *queue,
    void *state) {
  task_queue_notification_capture *capture =
      (task_queue_notification_capture *)state;
  if (capture == NULL) {
    return;
  }
  capture->last_queue = queue;
  capture->notifications += 1;
  if (capture->drain_on_notify) {
    tra_ffic_task_drain_finalization(queue);
  }
}

static void task_queue_notification_task(void *task_data) {
  task_queue_notification_capture *capture =
      (task_queue_notification_capture *)task_data;
  capture->ran += 1;
}

static int run_task_queue_notification_test(test_drain_mode drain_mode) {
  task_queue_notification_capture capture;
  tra_ffic_task_queue queue;
  int passed = 1;

  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_task_queue_init(
                           &queue, task_queue_notification_callback,
                           &capture),
                       "failed to initialize notifying task queue");
  if (passed) {
    passed = expect_true(!tra_ffic_task_queue_schedule(&queue, NULL,
                                                       &capture),
                         "invalid notifying task unexpectedly scheduled") &&
             passed;
    passed = expect_true(capture.notifications == 0,
                         "invalid notifying task unexpectedly notified") &&
             passed;
    passed = expect_true(tra_ffic_task_queue_schedule(
                             &queue, task_queue_notification_task, &capture),
                         "failed to schedule first notifying task") &&
             passed;
    passed = expect_true(tra_ffic_task_queue_schedule(
                             &queue, task_queue_notification_task, &capture),
                         "failed to schedule second notifying task") &&
             passed;
    passed = expect_true(capture.notifications == 2 &&
                             capture.last_queue == &queue &&
                             capture.ran == 0,
                         "task queue notification mismatch") &&
             passed;
    passed = test_task_queue_drain(&queue, drain_mode) && passed;
    passed = expect_true(capture.ran == 2,
                         "notifying task queue did not run tasks") &&
             passed;
    tra_ffic_task_queue_destroy(&queue);
  }

  memset(&capture, 0, sizeof(capture));
  passed = passed && expect_true(tra_ffic_task_queue_init(&queue, NULL,
                                                          NULL),
                                 "failed to initialize quiet task queue");
  if (passed) {
    passed = expect_true(tra_ffic_task_queue_schedule(
                             &queue, task_queue_notification_task, &capture),
                         "failed to schedule quiet task") &&
             passed;
    passed = expect_true(capture.notifications == 0,
                         "quiet task queue unexpectedly notified") &&
             passed;
    passed = test_task_queue_drain(&queue, drain_mode) && passed;
    passed = expect_true(capture.ran == 1,
                         "quiet task queue did not run task") &&
             passed;
    tra_ffic_task_queue_destroy(&queue);
  }

  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_task_queue_init(
                           &queue, task_queue_notification_callback,
                           NULL),
                       "failed to initialize null-state notification queue");
  if (passed) {
    passed = expect_true(tra_ffic_task_queue_schedule(
                             &queue, task_queue_notification_task, &capture),
                         "failed to schedule null-state notification task") &&
             passed;
    passed = test_task_queue_drain(&queue, drain_mode) && passed;
    passed = expect_true(capture.notifications == 0 && capture.ran == 1,
                         "null-state notification queue mismatch") &&
             passed;
    tra_ffic_task_queue_destroy(&queue);
  }

  memset(&capture, 0, sizeof(capture));
  capture.drain_on_notify = 1;
  passed = passed &&
           expect_true(tra_ffic_task_queue_init(
                           &queue, task_queue_notification_callback,
                           &capture),
                       "failed to initialize draining notification queue");
  if (passed) {
    passed = expect_true(tra_ffic_task_queue_schedule(
                             &queue, task_queue_notification_task, &capture),
                         "failed to schedule draining notification task") &&
             passed;
    passed = expect_true(capture.notifications == 1 &&
                             capture.last_queue == &queue &&
                             capture.ran == 1,
                         "draining notification callback mismatch") &&
             passed;
    tra_ffic_task_queue_destroy(&queue);
  }

  return passed;
}

static void finalize_counter_callback(void *user_data) {
  finalize_counter *counter = (finalize_counter *)user_data;
  counter->count += 1;
}

static void finalize_counter_alt_callback(void *user_data) {
  finalize_counter *counter = (finalize_counter *)user_data;
  counter->count += 1;
}

static void echo_bool_function(tra_ffic_completion completion, bool value) {
  completion(&value, NULL);
}

static void echo_i8_function(tra_ffic_completion completion, int8_t value) {
  completion(&value, NULL);
}

static void echo_u8_function(tra_ffic_completion completion, uint8_t value) {
  completion(&value, NULL);
}

static void echo_i16_function(tra_ffic_completion completion, int16_t value) {
  completion(&value, NULL);
}

static void echo_u16_function(tra_ffic_completion completion, uint16_t value) {
  completion(&value, NULL);
}

static void echo_i32_function(tra_ffic_completion completion, int32_t value) {
  completion(&value, NULL);
}

static void add_i32_function(tra_ffic_completion completion,
                             int32_t a,
                             int32_t b) {
  int32_t result = a + b;
  completion(&result, NULL);
}

static void add_i32_pointer_list_function(tra_ffic_completion completion,
                                          const void *const *args) {
  const int32_t a = *(const int32_t *)args[0];
  const int32_t b = *(const int32_t *)args[1];
  int32_t result = a + b;
  completion(&result, NULL);
}

static void add_i32_pointer_list_closure(tra_ffic_completion completion,
                                         void *closure_state,
                                         const void *const *args) {
  const int32_t *offset = (const int32_t *)closure_state;
  const int32_t a = *(const int32_t *)args[0];
  const int32_t b = *(const int32_t *)args[1];
  int32_t result = *offset + a + b;
  completion(&result, NULL);
}

static void echo_u32_function(tra_ffic_completion completion, uint32_t value) {
  completion(&value, NULL);
}

static void echo_i64_function(tra_ffic_completion completion, int64_t value) {
  completion(&value, NULL);
}

static void echo_u64_function(tra_ffic_completion completion, uint64_t value) {
  completion(&value, NULL);
}

static void echo_f32_function(tra_ffic_completion completion, float value) {
  completion(&value, NULL);
}

static void echo_f64_function(tra_ffic_completion completion, double value) {
  completion(&value, NULL);
}

static void echo_pointer_function(tra_ffic_completion completion, void *value) {
  completion(&value, NULL);
}

static void echo_string_function(tra_ffic_completion completion,
                                 const char *value) {
  completion(&value, NULL);
}

static void echo_buffer_view_function(tra_ffic_completion completion,
                                      tra_ffic_buffer_view value) {
  if (value.data != NULL && value.size > 1u) {
    ((uint8_t *)value.data)[1] = 0x42u;
  }
  completion(&value, NULL);
}

static basic_struct_value transform_basic_struct(
    basic_struct_value value,
    int32_t delta) {
  value.number += delta;
  value.nested.flag = (uint8_t)(value.nested.flag + 1u);
  value.nested.ratio *= 2.0;
  value.total += (uint64_t)delta;
  return value;
}

static void basic_struct_completion_closure(
    tra_ffic_completion completion,
    void *closure_state,
    basic_struct_value value) {
  const int32_t delta = *(const int32_t *)closure_state;
  basic_struct_value result = transform_basic_struct(value, delta);
  completion(&result, NULL);
  memset(&result, 0, sizeof(result));
}

static basic_struct_value basic_struct_retval_closure(
    void *closure_state,
    basic_struct_value value) {
  const int32_t delta = *(const int32_t *)closure_state;
  return transform_basic_struct(value, delta);
}

static void capture_basic_struct_callback(
    void *user_data,
    basic_struct_value result,
    const tra_ffic_error *error) {
  basic_struct_capture *capture = (basic_struct_capture *)user_data;
  capture->count += 1;
  capture->value = result;
  capture->has_error = error != NULL;
  if (error != NULL) {
    (void)snprintf(capture->error_message, sizeof(capture->error_message),
                   "%s", error->message);
  }
}

static void echo_void_function(tra_ffic_completion completion) {
  completion(NULL, NULL);
}

static void closure_echo_i32_function(tra_ffic_completion completion,
                                      void *closure_state,
                                      int32_t value) {
  (void)closure_state;
  completion(&value, NULL);
}

static void closure_echo_i8_function(tra_ffic_completion completion,
                                     void *closure_state,
                                     int8_t value) {
  (void)closure_state;
  completion(&value, NULL);
}

static void closure_echo_u8_function(tra_ffic_completion completion,
                                     void *closure_state,
                                     uint8_t value) {
  (void)closure_state;
  completion(&value, NULL);
}

static void closure_echo_i16_function(tra_ffic_completion completion,
                                      void *closure_state,
                                      int16_t value) {
  (void)closure_state;
  completion(&value, NULL);
}

static void closure_echo_u16_function(tra_ffic_completion completion,
                                      void *closure_state,
                                      uint16_t value) {
  (void)closure_state;
  completion(&value, NULL);
}

static void closure_echo_i64_function(tra_ffic_completion completion,
                                      void *closure_state,
                                      int64_t value) {
  (void)closure_state;
  completion(&value, NULL);
}

static void closure_echo_u64_function(tra_ffic_completion completion,
                                      void *closure_state,
                                      uint64_t value) {
  (void)closure_state;
  completion(&value, NULL);
}

static void closure_echo_f32_function(tra_ffic_completion completion,
                                      void *closure_state,
                                      float value) {
  (void)closure_state;
  completion(&value, NULL);
}

static void closure_echo_f64_function(tra_ffic_completion completion,
                                      void *closure_state,
                                      double value) {
  (void)closure_state;
  completion(&value, NULL);
}

static void closure_echo_pointer_function(tra_ffic_completion completion,
                                          void *closure_state,
                                          void *value) {
  (void)closure_state;
  completion(&value, NULL);
}

static void closure_echo_string_function(tra_ffic_completion completion,
                                         void *closure_state,
                                         const char *value) {
  (void)closure_state;
  completion(&value, NULL);
}

static void closure_echo_buffer_view_function(
    tra_ffic_completion completion,
    void *closure_state,
    tra_ffic_buffer_view value) {
  (void)closure_state;
  if (value.data != NULL && value.size > 2u) {
    ((uint8_t *)value.data)[2] = 0x24u;
  }
  completion(&value, NULL);
}

static void return_stack_string_function(tra_ffic_completion completion) {
  char stack_string[32];
  const char *result = stack_string;
  (void)snprintf(stack_string, sizeof(stack_string), "%s", "copied-string");
  completion(&result, NULL);
  memset(stack_string, '?', sizeof(stack_string));
}

static void return_null_string_function(tra_ffic_completion completion) {
  const char *result = NULL;
  completion(&result, NULL);
}

static void return_missing_string_function(tra_ffic_completion completion) {
  completion(NULL, NULL);
}

static void return_invalid_buffer_view_function(
    tra_ffic_completion completion) {
  tra_ffic_buffer_view value;
  value.data = NULL;
  value.size = 1u;
  completion(&value, NULL);
}

static void return_nan_function(tra_ffic_completion completion) {
  double value = NAN;
  completion(&value, NULL);
}

static void return_nan_float_function(tra_ffic_completion completion) {
  float value = NAN;
  completion(&value, NULL);
}

static void return_infinity_function(tra_ffic_completion completion) {
  double value = INFINITY;
  completion(&value, NULL);
}

static void return_infinity_float_function(tra_ffic_completion completion) {
  float value = INFINITY;
  completion(&value, NULL);
}

static void fail_i32_function(tra_ffic_completion completion) {
  completion(NULL, "expected failure");
}

static void double_completion_function(tra_ffic_completion completion) {
  const char *first = "first";
  const char *second = "second";
  completion(&first, NULL);
  completion(&second, NULL);
}

static void add_one_function(tra_ffic_completion completion, int32_t value) {
  int32_t result = value + 1;
  completion(&result, NULL);
}

static void accept_function_pure(tra_ffic_completion completion,
                                 i32_func function) {
  int32_t result = 1;
  g_seen_function_argument = function;
  completion(&result, NULL);
}

static void call_args_function(tra_ffic_completion completion,
                               void *closure_state,
                               args_i32_func function) {
  nested_callback_state *state = (nested_callback_state *)closure_state;
  typed_capture capture;
  tra_ffic_completion nested_completion = NULL;
  tra_ffic_error error;
  int32_t arg = 41;
  const void *args[1];
  int32_t value = 0;
  memset(&capture, 0, sizeof(capture));
  if (!tra_ffic_side_create_completion_function(
          state->caller_side, &k_type_i32, capture_i32_callback,
          &nested_completion, &capture, &error)) {
    completion(NULL, error.message);
    return;
  }
  args[0] = &arg;
  function(nested_completion, args);
  if (!test_task_queue_drain(state->queue, state->drain_mode)) {
    completion(NULL, "args nested drain failed");
    return;
  }
  if (!tra_ffic_function_release(nested_completion, &error)) {
    completion(NULL, error.message);
    return;
  }
  if (!test_task_queue_drain(state->queue, state->drain_mode)) {
    completion(NULL, "args nested cleanup drain failed");
    return;
  }
  if (capture.count != 1 || capture.has_error) {
    completion(NULL, "args nested call failed");
    return;
  }
  value = capture.int32_value;
  completion(&value, NULL);
}

static void accept_function_raw(tra_ffic_completion completion,
                                void *closure_state,
                                const tra_ffic_value *args,
                                uint32_t arg_count) {
  typed_capture *capture = (typed_capture *)closure_state;
  int32_t result = 7;
  if (capture != NULL) {
    capture->count += 1;
  }
  if (args == NULL || arg_count != 1u ||
      args[0].kind != TRA_FFIC_TYPE_FUNCTION) {
    if (capture != NULL) {
      capture->has_error = 1;
    }
    completion(NULL, "raw function argument mismatch");
    return;
  }
  if (capture != NULL) {
    capture->function_value = (i32_func)args[0].as.function_value;
  }
  completion(&result, NULL);
}

static void accept_buffer_view_raw(tra_ffic_completion completion,
                                   void *closure_state,
                                   const tra_ffic_value *args,
                                   uint32_t arg_count) {
  typed_capture *capture = (typed_capture *)closure_state;
  tra_ffic_buffer_view result;
  if (capture != NULL) {
    capture->count += 1;
  }
  if (args == NULL || arg_count != 1u ||
      args[0].kind != TRA_FFIC_TYPE_BUFFER_VIEW) {
    if (capture != NULL) {
      capture->has_error = 1;
    }
    completion(NULL, "raw buffer view argument mismatch");
    return;
  }
  result = args[0].as.buffer_view_value;
  if (result.data != NULL && result.size > 3u) {
    ((uint8_t *)result.data)[3] = 0x18u;
  }
  if (capture != NULL) {
    capture->buffer_view_value = result;
  }
  completion(&result, NULL);
}

static void call_nested_function(tra_ffic_completion completion,
                                 void *closure_state,
                                 i32_func function) {
  nested_callback_state *state = (nested_callback_state *)closure_state;
  typed_capture capture;
  tra_ffic_completion nested_completion = NULL;
  tra_ffic_error error;
  int32_t value = 0;
  memset(&capture, 0, sizeof(capture));
  if (!tra_ffic_side_create_completion_function(
          state->caller_side, &k_type_i32, capture_i32_callback,
          &nested_completion, &capture, &error)) {
    completion(NULL, error.message);
    return;
  }
  function(nested_completion, 41);
  if (!test_task_queue_drain(state->queue, state->drain_mode)) {
    completion(NULL, "nested drain failed");
    return;
  }
  if (!tra_ffic_function_release(nested_completion, &error)) {
    completion(NULL, error.message);
    return;
  }
  if (!test_task_queue_drain(state->queue, state->drain_mode)) {
    completion(NULL, "nested cleanup drain failed");
    return;
  }
  if (capture.count != 1 || capture.has_error) {
    completion(NULL, "nested call failed");
    return;
  }
  value = capture.int32_value;
  completion(&value, NULL);
}

static void call_nested_function_arg_function(tra_ffic_completion completion,
                                              void *closure_state,
                                              function_arg_func function) {
  nested_function_arg_state *state =
      (nested_function_arg_state *)closure_state;
  typed_capture capture;
  tra_ffic_completion nested_completion = NULL;
  tra_ffic_error error;
  int32_t value = 0;
  if (state == NULL || state->caller_side == NULL || state->queue == NULL ||
      state->inner_function == NULL) {
    completion(NULL, "nested function argument state is invalid");
    return;
  }
  memset(&capture, 0, sizeof(capture));
  if (!tra_ffic_side_create_completion_function(
          state->caller_side, &k_type_i32, capture_i32_callback,
          &nested_completion, &capture, &error)) {
    completion(NULL, error.message);
    return;
  }
  function(nested_completion, state->inner_function);
  if (!test_task_queue_drain(state->queue, state->drain_mode)) {
    completion(NULL, "nested function argument drain failed");
    return;
  }
  if (!tra_ffic_function_release(nested_completion, &error)) {
    completion(NULL, error.message);
    return;
  }
  if (!test_task_queue_drain(state->queue, state->drain_mode)) {
    completion(NULL, "nested function argument cleanup drain failed");
    return;
  }
  if (capture.count != 1 || capture.has_error) {
    completion(NULL, "nested function argument call failed");
    return;
  }
  value = capture.int32_value;
  completion(&value, NULL);
}

static void return_function(tra_ffic_completion completion,
                            void *closure_state) {
  i32_func function = *(i32_func *)closure_state;
  completion(&function, NULL);
}

static const char k_retval_borrowed_string[] = "retval-borrowed";
static uint8_t k_retval_buffer_data[] = {7u, 8u, 9u};
static int g_retval_void_call_count = 0;

static int32_t retval_add_i32_function(int32_t a, int32_t b) {
  return a + b;
}

static int32_t retval_add_i32_pointer_list_function(
    const void *const *args) {
  const int32_t a = *(const int32_t *)args[0];
  const int32_t b = *(const int32_t *)args[1];
  return a + b;
}

static int32_t retval_add_one_function(int32_t value) {
  return value + 1;
}

static double retval_echo_f64_function(double value) {
  return value;
}

static void retval_void_function(void) {
  g_retval_void_call_count += 1;
}

static int32_t retval_closure_add_state_function(void *closure_state,
                                                 int32_t value) {
  const int32_t *offset = (const int32_t *)closure_state;
  return value + *offset;
}

static const char *retval_return_string_function(void) {
  return k_retval_borrowed_string;
}

static const char *retval_return_null_string_function(void) {
  return NULL;
}

static tra_ffic_buffer_view retval_return_buffer_view_function(void) {
  tra_ffic_buffer_view view;
  view.data = k_retval_buffer_data;
  view.size = sizeof(k_retval_buffer_data);
  return view;
}

static retval_i32_func retval_return_function_closure(void *closure_state) {
  return *(retval_i32_func *)closure_state;
}

static int32_t retval_accept_function(retval_i32_func callback) {
  return callback(10);
}

static void three_level_set_message(
    char *message,
    const char *text) {
  (void)snprintf(message, TRA_FFIC_ERROR_MESSAGE_CAPACITY, "%s", text);
}

static void capture_three_level_result_callback(
    void *user_data,
    const tra_ffic_result *result) {
  three_level_result_capture *capture =
      (three_level_result_capture *)user_data;
  capture->count += 1;
  capture->has_error = 0;
  capture->error_message[0] = '\0';
  capture->value = tra_ffic_value_void();
  if (result == NULL) {
    capture->has_error = 1;
    three_level_set_message(capture->error_message, "result is null");
    return;
  }
  if (!result->success) {
    capture->has_error = 1;
    three_level_set_message(capture->error_message, result->error_message);
    return;
  }
  capture->value = result->value;
}

static int three_level_value_equals(const tra_ffic_value *left,
                                    const tra_ffic_value *right) {
  if (left == NULL || right == NULL || left->kind != right->kind) {
    return 0;
  }
  switch (left->kind) {
    case TRA_FFIC_TYPE_VOID:
      return 1;
    case TRA_FFIC_TYPE_BOOL:
      return left->as.bool_value == right->as.bool_value;
    case TRA_FFIC_TYPE_INT8:
      return left->as.int8_value == right->as.int8_value;
    case TRA_FFIC_TYPE_UINT8:
      return left->as.uint8_value == right->as.uint8_value;
    case TRA_FFIC_TYPE_INT16:
      return left->as.int16_value == right->as.int16_value;
    case TRA_FFIC_TYPE_UINT16:
      return left->as.uint16_value == right->as.uint16_value;
    case TRA_FFIC_TYPE_INT32:
      return left->as.int32_value == right->as.int32_value;
    case TRA_FFIC_TYPE_UINT32:
      return left->as.uint32_value == right->as.uint32_value;
    case TRA_FFIC_TYPE_INT64:
      return left->as.int64_value == right->as.int64_value;
    case TRA_FFIC_TYPE_UINT64:
      return left->as.uint64_value == right->as.uint64_value;
    case TRA_FFIC_TYPE_FLOAT:
      return left->as.float_value == right->as.float_value;
    case TRA_FFIC_TYPE_DOUBLE:
      return left->as.double_value == right->as.double_value;
    case TRA_FFIC_TYPE_POINTER:
      return left->as.pointer_value == right->as.pointer_value;
    case TRA_FFIC_TYPE_BUFFER_VIEW:
      return left->as.buffer_view_value.data ==
                 right->as.buffer_view_value.data &&
             left->as.buffer_view_value.size ==
                 right->as.buffer_view_value.size;
    case TRA_FFIC_TYPE_STRING:
      if (left->as.string_value == NULL || right->as.string_value == NULL) {
        return left->as.string_value == right->as.string_value;
      }
      return strcmp(left->as.string_value, right->as.string_value) == 0;
    case TRA_FFIC_TYPE_FUNCTION:
      return left->as.function_value == right->as.function_value;
    case TRA_FFIC_TYPE_STRUCT:
      return left->as.struct_value == right->as.struct_value;
  }
  return 0;
}

static void three_level_complete_value(tra_ffic_completion completion,
                                       const tra_ffic_value *value) {
  if (value == NULL) {
    completion(NULL, "three-level result is null");
    return;
  }
  switch (value->kind) {
    case TRA_FFIC_TYPE_VOID:
      completion(NULL, NULL);
      break;
    case TRA_FFIC_TYPE_BOOL:
      completion(&value->as.bool_value, NULL);
      break;
    case TRA_FFIC_TYPE_INT8:
      completion(&value->as.int8_value, NULL);
      break;
    case TRA_FFIC_TYPE_UINT8:
      completion(&value->as.uint8_value, NULL);
      break;
    case TRA_FFIC_TYPE_INT16:
      completion(&value->as.int16_value, NULL);
      break;
    case TRA_FFIC_TYPE_UINT16:
      completion(&value->as.uint16_value, NULL);
      break;
    case TRA_FFIC_TYPE_INT32:
      completion(&value->as.int32_value, NULL);
      break;
    case TRA_FFIC_TYPE_UINT32:
      completion(&value->as.uint32_value, NULL);
      break;
    case TRA_FFIC_TYPE_INT64:
      completion(&value->as.int64_value, NULL);
      break;
    case TRA_FFIC_TYPE_UINT64:
      completion(&value->as.uint64_value, NULL);
      break;
    case TRA_FFIC_TYPE_FLOAT:
      completion(&value->as.float_value, NULL);
      break;
    case TRA_FFIC_TYPE_DOUBLE:
      completion(&value->as.double_value, NULL);
      break;
    case TRA_FFIC_TYPE_POINTER:
      completion(&value->as.pointer_value, NULL);
      break;
    case TRA_FFIC_TYPE_BUFFER_VIEW:
      completion(&value->as.buffer_view_value, NULL);
      break;
    case TRA_FFIC_TYPE_STRING:
      completion(&value->as.string_value, NULL);
      break;
    case TRA_FFIC_TYPE_FUNCTION:
      completion(&value->as.function_value, NULL);
      break;
    case TRA_FFIC_TYPE_STRUCT:
      completion(value->as.struct_value, NULL);
      break;
  }
}

static void three_level_configure_terminal(
    three_level_case_state *state) {
  switch (state->terminal_case) {
    case THREE_LEVEL_I32_TO_F64:
      state->terminal_arg_type = &k_type_i32;
      state->terminal_return_type = &k_type_f64;
      state->terminal_arg_value = tra_ffic_value_int32(42);
      state->expected_result = tra_ffic_value_double(42.5);
      break;
    case THREE_LEVEL_U8_TO_I16:
      state->terminal_arg_type = &k_type_u8;
      state->terminal_return_type = &k_type_i16;
      state->terminal_arg_value = tra_ffic_value_uint8(250u);
      state->expected_result = tra_ffic_value_int16(-5);
      break;
    case THREE_LEVEL_F32_TO_BOOL:
      state->terminal_arg_type = &k_type_f32;
      state->terminal_return_type = &k_type_bool;
      state->terminal_arg_value = tra_ffic_value_float(-0.5f);
      state->expected_result = tra_ffic_value_bool(true);
      break;
    case THREE_LEVEL_F64_TO_I32:
      state->terminal_arg_type = &k_type_f64;
      state->terminal_return_type = &k_type_i32;
      state->terminal_arg_value = tra_ffic_value_double(12.5);
      state->expected_result = tra_ffic_value_int32(50);
      break;
    case THREE_LEVEL_BOOL_TO_U64:
      state->terminal_arg_type = &k_type_bool;
      state->terminal_return_type = &k_type_u64;
      state->terminal_arg_value = tra_ffic_value_bool(true);
      state->expected_result =
          tra_ffic_value_uint64(UINT64_C(0x1020304050607080));
      break;
    case THREE_LEVEL_U16_TO_F32:
      state->terminal_arg_type = &k_type_u16;
      state->terminal_return_type = &k_type_f32;
      state->terminal_arg_value = tra_ffic_value_uint16(9u);
      state->expected_result = tra_ffic_value_float(13.5f);
      break;
    case THREE_LEVEL_I64_TO_U32:
      state->terminal_arg_type = &k_type_i64;
      state->terminal_return_type = &k_type_u32;
      state->terminal_arg_value = tra_ffic_value_int64(INT64_C(9876543210));
      state->expected_result = tra_ffic_value_uint32(43227u);
      break;
    case THREE_LEVEL_U32_TO_I8:
      state->terminal_arg_type = &k_type_u32;
      state->terminal_return_type = &k_type_i8;
      state->terminal_arg_value = tra_ffic_value_uint32(250u);
      state->expected_result = tra_ffic_value_int8(-5);
      break;
  }
}

static void three_level_signature_set_init(
    three_level_signature_set *signature_set,
    const char *pattern,
    const tra_ffic_type *terminal_arg_type,
    const tra_ffic_type *terminal_return_type) {
  int index = 0;
  memset(signature_set, 0, sizeof(*signature_set));
  signature_set->terminal_arg_types[0] = *terminal_arg_type;
  signature_set->signatures[3].abi = TRA_FFIC_SIGNATURE_ABI_COMPLETION;
  signature_set->signatures[3].arg_count = 1u;
  signature_set->signatures[3].arg_types =
      signature_set->terminal_arg_types;
  signature_set->signatures[3].return_type = terminal_return_type;
  signature_set->function_types[3] =
      tra_ffic_type_function(&signature_set->signatures[3]);
  for (index = 2; index >= 0; --index) {
    if (pattern[index] == 'A') {
      signature_set->arg_types[index][0] =
          signature_set->function_types[index + 1];
      signature_set->signatures[index].abi = TRA_FFIC_SIGNATURE_ABI_COMPLETION;
      signature_set->signatures[index].arg_count = 1u;
      signature_set->signatures[index].arg_types =
          signature_set->arg_types[index];
      signature_set->signatures[index].return_type = terminal_return_type;
    } else {
      signature_set->signatures[index].abi = TRA_FFIC_SIGNATURE_ABI_COMPLETION;
      signature_set->signatures[index].arg_count = 0u;
      signature_set->signatures[index].arg_types = NULL;
      signature_set->signatures[index].return_type =
          &signature_set->function_types[index + 1];
    }
    signature_set->function_types[index] =
        tra_ffic_type_function(&signature_set->signatures[index]);
  }
}

static int three_level_call_with_result(
    three_level_case_state *state,
    tra_ffic_side *caller_side,
    const tra_ffic_function_ref *target_ref,
    const tra_ffic_value *args,
    uint32_t arg_count,
    three_level_result_capture *capture,
    char *message) {
  tra_ffic_error error;
  memset(capture, 0, sizeof(*capture));
  capture->value = tra_ffic_value_void();
  if (!tra_ffic_call_with_result(caller_side, target_ref, args, arg_count,
                                 capture_three_level_result_callback,
                                 capture, &error)) {
    three_level_set_message(message, error.message);
    return 0;
  }
  if (!test_task_queue_drain(state->queue, state->drain_mode)) {
    three_level_set_message(message, "three-level drain failed");
    return 0;
  }
  if (capture->count != 1) {
    three_level_set_message(message, "three-level result was not delivered");
    return 0;
  }
  if (capture->has_error) {
    three_level_set_message(message, capture->error_message);
    return 0;
  }
  return 1;
}

static int three_level_invoke_level(
    three_level_case_state *state,
    tra_ffic_side *caller_side,
    uint32_t index,
    tra_ffic_native_function function,
    three_level_result_capture *capture,
    char *message) {
  three_level_result_capture function_capture;
  tra_ffic_value arg_value;
  if (index > 3u) {
    three_level_set_message(message, "three-level index overflow");
    return 0;
  }
  if (function != state->refs[index].raw) {
    three_level_set_message(message, "three-level function pointer mismatch");
    return 0;
  }
  if (index == 3u) {
    arg_value = state->terminal_arg_value;
    return three_level_call_with_result(state, caller_side,
                                        &state->refs[index], &arg_value, 1u,
                                        capture, message);
  }
  if (state->pattern[index] == 'A') {
    arg_value = tra_ffic_value_function(state->refs[index + 1u].raw);
    return three_level_call_with_result(state, caller_side,
                                        &state->refs[index], &arg_value, 1u,
                                        capture, message);
  }
  if (state->pattern[index] == 'R') {
    if (!three_level_call_with_result(state, caller_side, &state->refs[index],
                                      NULL, 0u, &function_capture, message)) {
      return 0;
    }
    if (function_capture.value.kind != TRA_FFIC_TYPE_FUNCTION) {
      three_level_set_message(message, "three-level return was not function");
      return 0;
    }
    if (function_capture.value.as.function_value !=
        state->refs[index + 1u].raw) {
      three_level_set_message(message,
                              "three-level returned function mismatch");
      return 0;
    }
    return three_level_invoke_level(
        state, caller_side, index + 1u,
        function_capture.value.as.function_value, capture, message);
  }
  three_level_set_message(message, "three-level pattern is invalid");
  return 0;
}

static void three_level_terminal_raw(tra_ffic_completion completion,
                                     void *closure_state,
                                     const tra_ffic_value *args,
                                     uint32_t arg_count) {
  three_level_case_state *state =
      (three_level_case_state *)closure_state;
  tra_ffic_value result = tra_ffic_value_void();
  if (state == NULL || args == NULL || arg_count != 1u ||
      !three_level_value_equals(&args[0], &state->terminal_arg_value)) {
    completion(NULL, "three-level terminal argument mismatch");
    return;
  }
  switch (state->terminal_case) {
    case THREE_LEVEL_I32_TO_F64:
      result = tra_ffic_value_double((double)args[0].as.int32_value + 0.5);
      break;
    case THREE_LEVEL_U8_TO_I16:
      result = tra_ffic_value_int16((int16_t)args[0].as.uint8_value - 255);
      break;
    case THREE_LEVEL_F32_TO_BOOL:
      result = tra_ffic_value_bool(args[0].as.float_value < 0.0f);
      break;
    case THREE_LEVEL_F64_TO_I32:
      result = tra_ffic_value_int32((int32_t)(args[0].as.double_value * 4.0));
      break;
    case THREE_LEVEL_BOOL_TO_U64:
      result = tra_ffic_value_uint64(
          args[0].as.bool_value ? UINT64_C(0x1020304050607080) : 0u);
      break;
    case THREE_LEVEL_U16_TO_F32:
      result = tra_ffic_value_float((float)args[0].as.uint16_value * 1.5f);
      break;
    case THREE_LEVEL_I64_TO_U32:
      result = tra_ffic_value_uint32(
          (uint32_t)(args[0].as.int64_value % INT64_C(100000)) + 17u);
      break;
    case THREE_LEVEL_U32_TO_I8:
      result = tra_ffic_value_int8((int8_t)((int32_t)args[0].as.uint32_value -
                                           255));
      break;
  }
  three_level_complete_value(completion, &result);
}

static void three_level_node_raw(tra_ffic_completion completion,
                                 void *closure_state,
                                 const tra_ffic_value *args,
                                 uint32_t arg_count) {
  three_level_node_state *node = (three_level_node_state *)closure_state;
  three_level_case_state *state = NULL;
  three_level_result_capture capture;
  tra_ffic_native_function function = NULL;
  char message[TRA_FFIC_ERROR_MESSAGE_CAPACITY];
  message[0] = '\0';
  if (node == NULL || node->case_state == NULL || node->index > 2u) {
    completion(NULL, "three-level node state is invalid");
    return;
  }
  state = node->case_state;
  if (state->pattern[node->index] == 'R') {
    if (args != NULL || arg_count != 0u) {
      completion(NULL, "three-level return node received arguments");
      return;
    }
    function = state->refs[node->index + 1u].raw;
    completion(&function, NULL);
    return;
  }
  if (state->pattern[node->index] != 'A') {
    completion(NULL, "three-level node pattern is invalid");
    return;
  }
  if (args == NULL || arg_count != 1u ||
      args[0].kind != TRA_FFIC_TYPE_FUNCTION) {
    completion(NULL, "three-level argument node did not receive function");
    return;
  }
  if (args[0].as.function_value != state->refs[node->index + 1u].raw) {
    completion(NULL, "three-level argument function mismatch");
    return;
  }
  if (!three_level_invoke_level(state, state->callback_side,
                                node->index + 1u,
                                args[0].as.function_value, &capture,
                                message)) {
    completion(NULL, message);
    return;
  }
  three_level_complete_value(completion, &capture.value);
}

static void three_level_case_state_init(
    three_level_case_state *state,
    test_context *context,
    const char *name,
    const char *pattern,
    three_level_terminal_case terminal_case) {
  uint32_t index = 0u;
  memset(state, 0, sizeof(*state));
  state->name = name;
  state->pattern = pattern;
  state->terminal_case = terminal_case;
  state->invoke_side = &context->side_a;
  state->callback_side = &context->side_b;
  state->queue = &context->queue;
  state->drain_mode = context->drain_mode;
  three_level_configure_terminal(state);
  three_level_signature_set_init(&state->signature_set, state->pattern,
                                 state->terminal_arg_type,
                                 state->terminal_return_type);
  for (index = 0u; index < 4u; ++index) {
    state->refs[index].owner_side = &context->side_b;
    state->refs[index].signature =
        &state->signature_set.signatures[index];
  }
  for (index = 0u; index < 3u; ++index) {
    state->nodes[index].case_state = state;
    state->nodes[index].index = index;
  }
}

static int three_level_case_create_functions(
    three_level_case_state *state,
    tra_ffic_error *error) {
  int index = 0;
  if (!tra_ffic_side_create_raw_closure(
          state->callback_side, &state->signature_set.signatures[3],
          three_level_terminal_raw, state, NULL, &state->refs[3].raw,
          error)) {
    return 0;
  }
  for (index = 2; index >= 0; --index) {
    if (!tra_ffic_side_create_raw_closure(
            state->callback_side, &state->signature_set.signatures[index],
            three_level_node_raw, &state->nodes[index], NULL,
            &state->refs[index].raw, error)) {
      return 0;
    }
  }
  return 1;
}

static int three_level_case_release_functions(
    three_level_case_state *state,
    tra_ffic_error *error) {
  uint32_t index = 0u;
  int passed = 1;
  for (index = 0u; index < 4u; ++index) {
    if (state->refs[index].raw != NULL) {
      passed = expect_true(tra_ffic_function_release(state->refs[index].raw,
                                                     error),
                           error->message) &&
               passed;
      state->refs[index].raw = NULL;
    }
  }
  passed = test_task_queue_drain(state->queue, state->drain_mode) && passed;
  return passed;
}

static int run_three_level_function_signature_case(
    test_context *context,
    const char *name,
    const char *pattern,
    three_level_terminal_case terminal_case) {
  three_level_case_state state;
  three_level_result_capture capture;
  tra_ffic_error error;
  char message[TRA_FFIC_ERROR_MESSAGE_CAPACITY];
  int passed = 1;
  message[0] = '\0';
  three_level_case_state_init(&state, context, name, pattern, terminal_case);
  if (!expect_true(three_level_case_create_functions(&state, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  if (!three_level_invoke_level(&state, state.invoke_side, 0u,
                                state.refs[0].raw, &capture, message)) {
    passed = expect_true(0, message) && passed;
    goto cleanup;
  }
  (void)snprintf(message, sizeof(message),
                 "%s three-level terminal result mismatch", state.name);
  passed = passed &&
           expect_true(three_level_value_equals(&capture.value,
                                                &state.expected_result),
                       message);

cleanup:
  passed = three_level_case_release_functions(&state, &error) && passed;
  return passed;
}

static int run_three_level_function_signature_test(
    test_drain_mode drain_mode) {
  test_context context;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }
  passed = run_three_level_function_signature_case(
               &context, "argument-argument-argument", "AAA",
               THREE_LEVEL_I32_TO_F64) &&
           passed;
  passed = run_three_level_function_signature_case(
               &context, "argument-argument-return", "AAR",
               THREE_LEVEL_U8_TO_I16) &&
           passed;
  passed = run_three_level_function_signature_case(
               &context, "argument-return-argument", "ARA",
               THREE_LEVEL_F32_TO_BOOL) &&
           passed;
  passed = run_three_level_function_signature_case(
               &context, "argument-return-return", "ARR",
               THREE_LEVEL_F64_TO_I32) &&
           passed;
  passed = run_three_level_function_signature_case(
               &context, "return-argument-argument", "RAA",
               THREE_LEVEL_BOOL_TO_U64) &&
           passed;
  passed = run_three_level_function_signature_case(
               &context, "return-argument-return", "RAR",
               THREE_LEVEL_U16_TO_F32) &&
           passed;
  passed = run_three_level_function_signature_case(
               &context, "return-return-argument", "RRA",
               THREE_LEVEL_I64_TO_U32) &&
           passed;
  passed = run_three_level_function_signature_case(
               &context, "return-return-return", "RRR",
               THREE_LEVEL_U32_TO_I8) &&
           passed;
  passed = test_context_destroy(&context) && passed;
  return passed;
}

#if TRA_FFIC_IN_WIN32
static DWORD WINAPI async_thread_main(LPVOID user_data) {
#else
static void *async_thread_main(void *user_data) {
#endif
  async_thread_input *input = (async_thread_input *)user_data;
  int32_t value = input->value + 1;
  input->completion(&value, NULL);
  free(input);
#if TRA_FFIC_IN_WIN32
  return 0;
#else
  return NULL;
#endif
}

static void async_function(tra_ffic_completion completion,
                           void *closure_state,
                           int32_t value) {
  async_state *state = (async_state *)closure_state;
  async_thread_input *input = NULL;
  input = (async_thread_input *)calloc(1u, sizeof(*input));
  if (input == NULL) {
    completion(NULL, "async allocation failed");
    return;
  }
  input->completion = completion;
  input->value = value;
#if TRA_FFIC_IN_WIN32
  state->thread = CreateThread(NULL, 0, async_thread_main, input, 0, NULL);
  state->thread_started = state->thread != NULL;
  if (!state->thread_started) {
    free(input);
    completion(NULL, "CreateThread failed");
  }
#else
  state->thread_started = pthread_create(&state->thread, NULL,
                                         async_thread_main, input) == 0;
  if (!state->thread_started) {
    free(input);
    completion(NULL, "pthread_create failed");
  }
#endif
}

static void finalize_release_during_call_callback(void *user_data) {
  release_during_call_state *state = (release_during_call_state *)user_data;
  state->finalizer->count += 1;
}

static void release_during_call_function(tra_ffic_completion completion,
                                         void *closure_state,
                                         int32_t value) {
  release_during_call_state *state =
      (release_during_call_state *)closure_state;
  tra_ffic_error error;
  int32_t result = 0;
  if (!tra_ffic_function_release(state->self, &error)) {
    completion(NULL, error.message);
    return;
  }
  if (state->finalizer->count != 0) {
    completion(NULL, "closure finalized while trampoline was active");
    return;
  }
  result = value + 1;
  completion(&result, NULL);
}

static int run_readme_minimum_test(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  add_func function = NULL;
  tra_ffic_completion completion = NULL;
  typed_capture capture;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }
  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_sig_add_i32, add_i32_function,
                           &function, &error),
                       error.message);
  passed = passed &&
           expect_true(function == add_i32_function,
                       "pure primitive function was not returned directly");
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32,
                           capture_i32_callback, &completion, &capture,
                           &error),
                       error.message);
  function(completion, 40, 2);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 42,
                       "README-style direct call failed");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(function, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;
  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_primitive_test(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  tra_ffic_completion completion = NULL;
  typed_capture capture;
  bool_func bool_function = NULL;
  i8_func i8_function = NULL;
  u8_func u8_function = NULL;
  i16_func i16_function = NULL;
  u16_func u16_function = NULL;
  i32_func i32_function = NULL;
  u32_func u32_function = NULL;
  i64_func i64_function = NULL;
  u64_func u64_function = NULL;
  f32_func f32_function = NULL;
  f64_func f64_function = NULL;
  pointer_func pointer_function = NULL;
  string_func string_function = NULL;
  void_func void_function = NULL;
  int pointer_target = 123;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }

#define RUN_PRIMITIVE_CASE(signature, return_type, callback, completion_var,   \
                           capture_callback, call_expr, check_expr)            \
  do {                                                                         \
    memset(&capture, 0, sizeof(capture));                                      \
    passed = passed &&                                                         \
             expect_true(tra_ffic_side_create_pure_function(                   \
                             &context.side_b, (signature), (callback),         \
                             &(completion_var), &error),                       \
                         error.message);                                       \
    passed = passed &&                                                         \
             expect_true(tra_ffic_side_create_completion_function(             \
                             &context.side_a, (return_type),                   \
                             (capture_callback), &completion, &capture,        \
                             &error),                                          \
                         error.message);                                       \
    call_expr;                                                                 \
    passed = test_context_drain(&context) && passed;                                 \
    passed = passed && expect_true((check_expr), "primitive result mismatch"); \
    passed = passed &&                                                         \
             expect_true(tra_ffic_function_release(completion, &error),        \
                         error.message);                                       \
    passed = passed &&                                                         \
             expect_true(tra_ffic_function_release((completion_var), &error),  \
                         error.message);                                       \
    passed = test_context_drain(&context) && passed;                                 \
    completion = NULL;                                                         \
  } while (0)

  RUN_PRIMITIVE_CASE(&k_sig_echo_bool, &k_type_bool, echo_bool_function,
                     bool_function, capture_bool_callback,
                     bool_function(completion, false),
                     capture.count == 1 && !capture.has_error &&
                         !capture.bool_value);
  RUN_PRIMITIVE_CASE(&k_sig_echo_i8, &k_type_i8, echo_i8_function,
                     i8_function, capture_i8_callback,
                     i8_function(completion, INT8_MIN),
                     capture.count == 1 && !capture.has_error &&
                         capture.int8_value == INT8_MIN);
  RUN_PRIMITIVE_CASE(&k_sig_echo_u8, &k_type_u8, echo_u8_function,
                     u8_function, capture_u8_callback,
                     u8_function(completion, UINT8_MAX),
                     capture.count == 1 && !capture.has_error &&
                         capture.uint8_value == UINT8_MAX);
  RUN_PRIMITIVE_CASE(&k_sig_echo_i16, &k_type_i16, echo_i16_function,
                     i16_function, capture_i16_callback,
                     i16_function(completion, INT16_MIN),
                     capture.count == 1 && !capture.has_error &&
                         capture.int16_value == INT16_MIN);
  RUN_PRIMITIVE_CASE(&k_sig_echo_u16, &k_type_u16, echo_u16_function,
                     u16_function, capture_u16_callback,
                     u16_function(completion, UINT16_MAX),
                     capture.count == 1 && !capture.has_error &&
                         capture.uint16_value == UINT16_MAX);
  RUN_PRIMITIVE_CASE(&k_sig_echo_i32, &k_type_i32, echo_i32_function,
                     i32_function, capture_i32_callback,
                     i32_function(completion, -2147483647 - 1),
                     capture.count == 1 && !capture.has_error &&
                         capture.int32_value == (-2147483647 - 1));
  RUN_PRIMITIVE_CASE(&k_sig_echo_u32, &k_type_u32, echo_u32_function,
                     u32_function, capture_u32_callback,
                     u32_function(completion, 4294967295u),
                     capture.count == 1 && !capture.has_error &&
                         capture.uint32_value == 4294967295u);
  RUN_PRIMITIVE_CASE(&k_sig_echo_i64, &k_type_i64, echo_i64_function,
                     i64_function, capture_i64_callback,
                     i64_function(completion, INT64_MIN),
                     capture.count == 1 && !capture.has_error &&
                         capture.int64_value == INT64_MIN);
  RUN_PRIMITIVE_CASE(&k_sig_echo_u64, &k_type_u64, echo_u64_function,
                     u64_function, capture_u64_callback,
                     u64_function(completion, UINT64_MAX),
                     capture.count == 1 && !capture.has_error &&
                         capture.uint64_value == UINT64_MAX);
  RUN_PRIMITIVE_CASE(&k_sig_echo_f32, &k_type_f32, echo_f32_function,
                     f32_function, capture_f32_callback,
                     f32_function(completion, -12.5f),
                     capture.count == 1 && !capture.has_error &&
                         capture.float_value == -12.5f);
  RUN_PRIMITIVE_CASE(&k_sig_echo_f32, &k_type_f32, echo_f32_function,
                     f32_function, capture_f32_callback,
                     f32_function(completion, NAN),
                     capture.count == 1 && !capture.has_error &&
                         isnan(capture.float_value));
  RUN_PRIMITIVE_CASE(&k_sig_echo_f32, &k_type_f32, echo_f32_function,
                     f32_function, capture_f32_callback,
                     f32_function(completion, INFINITY),
                     capture.count == 1 && !capture.has_error &&
                         is_positive_infinity_float(capture.float_value));
  RUN_PRIMITIVE_CASE(&k_sig_echo_f64, &k_type_f64, echo_f64_function,
                     f64_function, capture_f64_callback,
                     f64_function(completion, 1.25),
                     capture.count == 1 && !capture.has_error &&
                         capture.double_value == 1.25);
  RUN_PRIMITIVE_CASE(&k_sig_echo_f64, &k_type_f64, echo_f64_function,
                     f64_function, capture_f64_callback,
                     f64_function(completion, NAN),
                     capture.count == 1 && !capture.has_error &&
                         isnan(capture.double_value));
  RUN_PRIMITIVE_CASE(&k_sig_echo_f64, &k_type_f64, echo_f64_function,
                     f64_function, capture_f64_callback,
                     f64_function(completion, INFINITY),
                     capture.count == 1 && !capture.has_error &&
                         is_positive_infinity_double(capture.double_value));
  RUN_PRIMITIVE_CASE(&k_sig_echo_pointer, &k_type_pointer,
                     echo_pointer_function, pointer_function,
                     capture_pointer_callback,
                     pointer_function(completion, &pointer_target),
                     capture.count == 1 && !capture.has_error &&
                         capture.pointer_value == &pointer_target);
  RUN_PRIMITIVE_CASE(&k_sig_echo_pointer, &k_type_pointer,
                     echo_pointer_function, pointer_function,
                     capture_pointer_callback,
                     pointer_function(completion, NULL),
                     capture.count == 1 && !capture.has_error &&
                         capture.pointer_value == NULL);
  RUN_PRIMITIVE_CASE(&k_sig_echo_string, &k_type_string, echo_string_function,
                     string_function, capture_string_callback,
                     string_function(completion, "hello"),
                     capture.count == 1 && !capture.has_error &&
                         strcmp(capture.string_value, "hello") == 0);
  RUN_PRIMITIVE_CASE(&k_sig_echo_string, &k_type_string, echo_string_function,
                     string_function, capture_string_callback,
                     string_function(completion, NULL),
                     capture.count == 1 && !capture.has_error &&
                         capture.string_was_null);
  RUN_PRIMITIVE_CASE(&k_sig_void, &k_type_void, echo_void_function,
                     void_function, capture_void_callback,
                     void_function(completion),
                     capture.count == 1 && !capture.has_error);

#undef RUN_PRIMITIVE_CASE

  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_basic_struct_test(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  tra_ffic_type inner_field_types[2];
  tra_ffic_type struct_field_types[3];
  tra_ffic_type arg_types[1];
  tra_ffic_type inner_type;
  tra_ffic_type struct_type;
  tra_ffic_signature completion_signature;
  tra_ffic_signature retval_signature;
  basic_struct_func completion_function = NULL;
  retval_basic_struct_func retval_function = NULL;
  tra_ffic_completion completion = NULL;
  basic_struct_capture capture;
  basic_struct_value input;
  basic_struct_value result;
  int32_t delta = 5;
  int passed = 1;

  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }

  inner_field_types[0] = tra_ffic_type_uint8();
  inner_field_types[1] = tra_ffic_type_double();
  inner_type = tra_ffic_type_struct(2u, inner_field_types);
  struct_field_types[0] = tra_ffic_type_int32();
  struct_field_types[1] = inner_type;
  struct_field_types[2] = tra_ffic_type_uint64();
  struct_type = tra_ffic_type_struct(3u, struct_field_types);
  arg_types[0] = struct_type;
  completion_signature = tra_ffic_signature_stack(
      TRA_FFIC_SIGNATURE_ABI_COMPLETION, 1u, arg_types, &struct_type);
  retval_signature = tra_ffic_signature_stack(
      TRA_FFIC_SIGNATURE_ABI_RETVAL, 1u, arg_types, &struct_type);

  memset(&capture, 0, sizeof(capture));
  passed = expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &completion_signature,
                           basic_struct_completion_closure, &delta, NULL,
                           &completion_function, &error),
                       error.message) &&
           passed;
  passed = expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &struct_type,
                           capture_basic_struct_callback, &completion,
                           &capture, &error),
                       error.message) &&
           passed;
  passed = expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &retval_signature,
                           basic_struct_retval_closure, &delta, NULL,
                           &retval_function, &error),
                       error.message) &&
           passed;

  memset(inner_field_types, 0, sizeof(inner_field_types));
  memset(struct_field_types, 0, sizeof(struct_field_types));
  memset(arg_types, 0, sizeof(arg_types));

  memset(&input, 0, sizeof(input));
  input.number = 37;
  input.nested.flag = 2u;
  input.nested.ratio = 1.25;
  input.total = 100u;
  if (completion_function != NULL && completion != NULL) {
    completion_function(completion, input);
    passed = test_context_drain(&context) && passed;
    passed = expect_true(
                 capture.count == 1 && !capture.has_error &&
                     capture.value.number == 42 &&
                     capture.value.nested.flag == 3u &&
                     capture.value.nested.ratio == 2.5 &&
                     capture.value.total == 105u,
                 "completion struct result mismatch") &&
             passed;
  }
  if (retval_function != NULL) {
    result = retval_function(input);
    passed = expect_true(result.number == 42 &&
                             result.nested.flag == 3u &&
                             result.nested.ratio == 2.5 &&
                             result.total == 105u,
                         "retval struct result mismatch") &&
             passed;
  }

  if (completion != NULL) {
    passed = expect_true(tra_ffic_function_release(completion, &error),
                         error.message) &&
             passed;
  }
  if (completion_function != NULL) {
    passed = expect_true(
                 tra_ffic_function_release(completion_function, &error),
                 error.message) &&
             passed;
  }
  if (retval_function != NULL) {
    passed = expect_true(tra_ffic_function_release(retval_function, &error),
                         error.message) &&
             passed;
  }
  passed = test_context_drain(&context) && passed;
  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_closure_scalar_test(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  tra_ffic_completion completion = NULL;
  typed_capture capture;
  i8_func i8_function = NULL;
  u8_func u8_function = NULL;
  i16_func i16_function = NULL;
  u16_func u16_function = NULL;
  i64_func i64_function = NULL;
  u64_func u64_function = NULL;
  f32_func f32_function = NULL;
  pointer_func pointer_function = NULL;
  int pointer_target = 456;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }

#define RUN_CLOSURE_CASE(signature, return_type, callback, function_var,        \
                         capture_callback, call_expr, check_expr)              \
  do {                                                                         \
    memset(&capture, 0, sizeof(capture));                                      \
    passed = passed &&                                                         \
             expect_true(tra_ffic_side_create_closure(                         \
                             &context.side_b, (signature), (callback), NULL,   \
                             NULL, &(function_var), &error),                   \
                         error.message);                                       \
    passed = passed &&                                                         \
             expect_true(tra_ffic_side_create_completion_function(             \
                             &context.side_a, (return_type),                   \
                             (capture_callback), &completion, &capture,        \
                             &error),                                          \
                         error.message);                                       \
    call_expr;                                                                 \
    passed = test_context_drain(&context) && passed;                           \
    passed = passed && expect_true((check_expr), "closure result mismatch");   \
    passed = passed &&                                                         \
             expect_true(tra_ffic_function_release(completion, &error),        \
                         error.message);                                       \
    passed = passed &&                                                         \
             expect_true(tra_ffic_function_release((function_var), &error),    \
                         error.message);                                       \
    passed = test_context_drain(&context) && passed;                           \
    completion = NULL;                                                         \
    function_var = NULL;                                                       \
  } while (0)

  RUN_CLOSURE_CASE(&k_sig_echo_i8, &k_type_i8, closure_echo_i8_function,
                   i8_function, capture_i8_callback,
                   i8_function(completion, INT8_MIN),
                   capture.count == 1 && !capture.has_error &&
                       capture.int8_value == INT8_MIN);
  RUN_CLOSURE_CASE(&k_sig_echo_u8, &k_type_u8, closure_echo_u8_function,
                   u8_function, capture_u8_callback,
                   u8_function(completion, UINT8_MAX),
                   capture.count == 1 && !capture.has_error &&
                       capture.uint8_value == UINT8_MAX);
  RUN_CLOSURE_CASE(&k_sig_echo_i16, &k_type_i16, closure_echo_i16_function,
                   i16_function, capture_i16_callback,
                   i16_function(completion, INT16_MIN),
                   capture.count == 1 && !capture.has_error &&
                       capture.int16_value == INT16_MIN);
  RUN_CLOSURE_CASE(&k_sig_echo_u16, &k_type_u16, closure_echo_u16_function,
                   u16_function, capture_u16_callback,
                   u16_function(completion, UINT16_MAX),
                   capture.count == 1 && !capture.has_error &&
                       capture.uint16_value == UINT16_MAX);
  RUN_CLOSURE_CASE(&k_sig_echo_i64, &k_type_i64, closure_echo_i64_function,
                   i64_function, capture_i64_callback,
                   i64_function(completion, INT64_MIN),
                   capture.count == 1 && !capture.has_error &&
                       capture.int64_value == INT64_MIN);
  RUN_CLOSURE_CASE(&k_sig_echo_u64, &k_type_u64, closure_echo_u64_function,
                   u64_function, capture_u64_callback,
                   u64_function(completion, UINT64_MAX),
                   capture.count == 1 && !capture.has_error &&
                       capture.uint64_value == UINT64_MAX);
  RUN_CLOSURE_CASE(&k_sig_echo_f32, &k_type_f32, closure_echo_f32_function,
                   f32_function, capture_f32_callback,
                   f32_function(completion, -12.5f),
                   capture.count == 1 && !capture.has_error &&
                       capture.float_value == -12.5f);
  RUN_CLOSURE_CASE(&k_sig_echo_pointer, &k_type_pointer,
                   closure_echo_pointer_function, pointer_function,
                   capture_pointer_callback,
                   pointer_function(completion, &pointer_target),
                   capture.count == 1 && !capture.has_error &&
                       capture.pointer_value == &pointer_target);
  RUN_CLOSURE_CASE(&k_sig_echo_pointer, &k_type_pointer,
                   closure_echo_pointer_function, pointer_function,
                   capture_pointer_callback,
                   pointer_function(completion, NULL),
                   capture.count == 1 && !capture.has_error &&
                       capture.pointer_value == NULL);

#undef RUN_CLOSURE_CASE

  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_structured_scalar_call_test(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  typed_capture capture;
  i8_func i8_function = NULL;
  u8_func u8_function = NULL;
  i16_func i16_function = NULL;
  u16_func u16_function = NULL;
  i64_func i64_function = NULL;
  u64_func u64_function = NULL;
  f32_func f32_function = NULL;
  f64_func f64_function = NULL;
  pointer_func pointer_function = NULL;
  string_func string_function = NULL;
  int pointer_target = 789;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }

#define RUN_STRUCTURED_CASE(case_signature, callback, function_var,             \
                            arg_value_expr, check_expr)                        \
  do {                                                                         \
    tra_ffic_function_ref target_ref;                                          \
    tra_ffic_value arg_value;                                                  \
    int case_ok = 0;                                                           \
    memset(&capture, 0, sizeof(capture));                                      \
    memset(&target_ref, 0, sizeof(target_ref));                                \
    arg_value = (arg_value_expr);                                              \
    case_ok = expect_true(tra_ffic_side_create_closure(                        \
                              &context.side_b, (case_signature), (callback),   \
                              NULL, NULL, &(function_var), &error),            \
                          error.message);                                      \
    passed = case_ok && passed;                                                \
    if (case_ok) {                                                             \
      target_ref.raw = (tra_ffic_native_function)(function_var);               \
      target_ref.owner_side = &context.side_b;                                 \
      target_ref.signature = (case_signature);                                 \
      passed = expect_true(tra_ffic_call_with_result(                          \
                               &context.side_a, &target_ref, &arg_value, 1u,   \
                               capture_result_callback, &capture, &error),     \
                           error.message) &&                                   \
               passed;                                                         \
      passed = test_context_drain(&context) && passed;                         \
      passed = passed &&                                                       \
               expect_true((check_expr), "structured result mismatch");        \
      passed = expect_true(tra_ffic_function_release((function_var), &error),  \
                           error.message) &&                                   \
               passed;                                                         \
      passed = test_context_drain(&context) && passed;                         \
      function_var = NULL;                                                     \
    }                                                                          \
  } while (0)

  RUN_STRUCTURED_CASE(&k_sig_echo_i8, closure_echo_i8_function, i8_function,
                      tra_ffic_value_int8(INT8_MIN),
                      capture.count == 1 && !capture.has_error &&
                          capture.int8_value == INT8_MIN);
  RUN_STRUCTURED_CASE(&k_sig_echo_u8, closure_echo_u8_function, u8_function,
                      tra_ffic_value_uint8(UINT8_MAX),
                      capture.count == 1 && !capture.has_error &&
                          capture.uint8_value == UINT8_MAX);
  RUN_STRUCTURED_CASE(&k_sig_echo_i16, closure_echo_i16_function, i16_function,
                      tra_ffic_value_int16(INT16_MIN),
                      capture.count == 1 && !capture.has_error &&
                          capture.int16_value == INT16_MIN);
  RUN_STRUCTURED_CASE(&k_sig_echo_u16, closure_echo_u16_function, u16_function,
                      tra_ffic_value_uint16(UINT16_MAX),
                      capture.count == 1 && !capture.has_error &&
                          capture.uint16_value == UINT16_MAX);
  RUN_STRUCTURED_CASE(&k_sig_echo_i64, closure_echo_i64_function, i64_function,
                      tra_ffic_value_int64(INT64_MIN),
                      capture.count == 1 && !capture.has_error &&
                          capture.int64_value == INT64_MIN);
  RUN_STRUCTURED_CASE(&k_sig_echo_u64, closure_echo_u64_function, u64_function,
                      tra_ffic_value_uint64(UINT64_MAX),
                      capture.count == 1 && !capture.has_error &&
                          capture.uint64_value == UINT64_MAX);
  RUN_STRUCTURED_CASE(&k_sig_echo_f32, closure_echo_f32_function, f32_function,
                      tra_ffic_value_float(-12.5f),
                      capture.count == 1 && !capture.has_error &&
                          capture.float_value == -12.5f);
  RUN_STRUCTURED_CASE(&k_sig_echo_f32, closure_echo_f32_function, f32_function,
                      tra_ffic_value_float(NAN),
                      capture.count == 1 && !capture.has_error &&
                          isnan(capture.float_value));
  RUN_STRUCTURED_CASE(&k_sig_echo_f32, closure_echo_f32_function, f32_function,
                      tra_ffic_value_float(INFINITY),
                      capture.count == 1 && !capture.has_error &&
                          is_positive_infinity_float(capture.float_value));
  RUN_STRUCTURED_CASE(&k_sig_echo_f64, closure_echo_f64_function, f64_function,
                      tra_ffic_value_double(NAN),
                      capture.count == 1 && !capture.has_error &&
                          isnan(capture.double_value));
  RUN_STRUCTURED_CASE(&k_sig_echo_f64, closure_echo_f64_function, f64_function,
                      tra_ffic_value_double(INFINITY),
                      capture.count == 1 && !capture.has_error &&
                          is_positive_infinity_double(capture.double_value));
  RUN_STRUCTURED_CASE(&k_sig_echo_pointer, closure_echo_pointer_function,
                      pointer_function,
                      tra_ffic_value_pointer(&pointer_target),
                      capture.count == 1 && !capture.has_error &&
                          capture.pointer_value == &pointer_target);
  RUN_STRUCTURED_CASE(&k_sig_echo_pointer, closure_echo_pointer_function,
                      pointer_function, tra_ffic_value_pointer(NULL),
                      capture.count == 1 && !capture.has_error &&
                          capture.pointer_value == NULL);
  RUN_STRUCTURED_CASE(&k_sig_echo_string, closure_echo_string_function,
                      string_function, tra_ffic_value_string(NULL),
                      capture.count == 1 && !capture.has_error &&
                          capture.string_was_null);

#undef RUN_STRUCTURED_CASE

  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_success_scalar_call_test(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  typed_capture capture;
  i8_func i8_function = NULL;
  u8_func u8_function = NULL;
  i16_func i16_function = NULL;
  u16_func u16_function = NULL;
  i64_func i64_function = NULL;
  u64_func u64_function = NULL;
  f32_func f32_function = NULL;
  f64_func f64_function = NULL;
  pointer_func pointer_function = NULL;
  string_func string_function = NULL;
  int pointer_target = 987;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }

#define RUN_SUCCESS_CALL_CASE(case_signature, callback, function_var,           \
                              arg_value_expr, success_callback, check_expr)    \
  do {                                                                         \
    tra_ffic_function_ref target_ref;                                          \
    tra_ffic_value arg_value;                                                  \
    int case_ok = 0;                                                           \
    memset(&capture, 0, sizeof(capture));                                      \
    memset(&target_ref, 0, sizeof(target_ref));                                \
    arg_value = (arg_value_expr);                                              \
    case_ok = expect_true(tra_ffic_side_create_closure(                        \
                              &context.side_b, (case_signature), (callback),   \
                              NULL, NULL, &(function_var), &error),            \
                          error.message);                                      \
    passed = case_ok && passed;                                                \
    if (case_ok) {                                                             \
      target_ref.raw = (tra_ffic_native_function)(function_var);               \
      target_ref.owner_side = &context.side_b;                                 \
      target_ref.signature = (case_signature);                                 \
      passed = expect_true(tra_ffic_call(                                      \
                               &context.side_a, &target_ref, &arg_value, 1u,   \
                               (success_callback), &capture, &error),          \
                           error.message) &&                                   \
               passed;                                                         \
      passed = test_context_drain(&context) && passed;                         \
      passed = passed &&                                                       \
               expect_true((check_expr), "success call result mismatch");      \
      passed = expect_true(tra_ffic_function_release((function_var), &error),  \
                           error.message) &&                                   \
               passed;                                                         \
      passed = test_context_drain(&context) && passed;                         \
      function_var = NULL;                                                     \
    }                                                                          \
  } while (0)

  RUN_SUCCESS_CALL_CASE(&k_sig_echo_i8, closure_echo_i8_function, i8_function,
                        tra_ffic_value_int8(INT8_MIN),
                        capture_i8_success_callback,
                        capture.count == 1 && capture.int8_value == INT8_MIN);
  RUN_SUCCESS_CALL_CASE(&k_sig_echo_u8, closure_echo_u8_function, u8_function,
                        tra_ffic_value_uint8(UINT8_MAX),
                        capture_u8_success_callback,
                        capture.count == 1 && capture.uint8_value == UINT8_MAX);
  RUN_SUCCESS_CALL_CASE(&k_sig_echo_i16, closure_echo_i16_function,
                        i16_function, tra_ffic_value_int16(INT16_MIN),
                        capture_i16_success_callback,
                        capture.count == 1 && capture.int16_value == INT16_MIN);
  RUN_SUCCESS_CALL_CASE(&k_sig_echo_u16, closure_echo_u16_function,
                        u16_function, tra_ffic_value_uint16(UINT16_MAX),
                        capture_u16_success_callback,
                        capture.count == 1 && capture.uint16_value == UINT16_MAX);
  RUN_SUCCESS_CALL_CASE(&k_sig_echo_i64, closure_echo_i64_function,
                        i64_function, tra_ffic_value_int64(INT64_MIN),
                        capture_i64_success_callback,
                        capture.count == 1 && capture.int64_value == INT64_MIN);
  RUN_SUCCESS_CALL_CASE(&k_sig_echo_u64, closure_echo_u64_function,
                        u64_function, tra_ffic_value_uint64(UINT64_MAX),
                        capture_u64_success_callback,
                        capture.count == 1 && capture.uint64_value == UINT64_MAX);
  RUN_SUCCESS_CALL_CASE(&k_sig_echo_f32, closure_echo_f32_function,
                        f32_function, tra_ffic_value_float(-12.5f),
                        capture_f32_success_callback,
                        capture.count == 1 && capture.float_value == -12.5f);
  RUN_SUCCESS_CALL_CASE(&k_sig_echo_f32, closure_echo_f32_function,
                        f32_function, tra_ffic_value_float(NAN),
                        capture_f32_success_callback,
                        capture.count == 1 && isnan(capture.float_value));
  RUN_SUCCESS_CALL_CASE(&k_sig_echo_f32, closure_echo_f32_function,
                        f32_function, tra_ffic_value_float(INFINITY),
                        capture_f32_success_callback,
                        capture.count == 1 &&
                            is_positive_infinity_float(capture.float_value));
  RUN_SUCCESS_CALL_CASE(&k_sig_echo_f64, closure_echo_f64_function,
                        f64_function, tra_ffic_value_double(NAN),
                        capture_f64_success_callback,
                        capture.count == 1 && isnan(capture.double_value));
  RUN_SUCCESS_CALL_CASE(&k_sig_echo_f64, closure_echo_f64_function,
                        f64_function, tra_ffic_value_double(INFINITY),
                        capture_f64_success_callback,
                        capture.count == 1 &&
                            is_positive_infinity_double(capture.double_value));
  RUN_SUCCESS_CALL_CASE(&k_sig_echo_pointer, closure_echo_pointer_function,
                        pointer_function,
                        tra_ffic_value_pointer(&pointer_target),
                        capture_pointer_success_callback,
                        capture.count == 1 &&
                            capture.pointer_value == &pointer_target);
  RUN_SUCCESS_CALL_CASE(&k_sig_echo_pointer, closure_echo_pointer_function,
                        pointer_function, tra_ffic_value_pointer(NULL),
                        capture_pointer_success_callback,
                        capture.count == 1 && capture.pointer_value == NULL);
  RUN_SUCCESS_CALL_CASE(&k_sig_echo_string, closure_echo_string_function,
                        string_function, tra_ffic_value_string(NULL),
                        capture_string_success_callback,
                        capture.count == 1 && capture.string_was_null);

#undef RUN_SUCCESS_CALL_CASE

  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_retval_abi_test(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  typed_capture capture;
  tra_ffic_function_ref target_ref;
  tra_ffic_value add_args[2];
  tra_ffic_value one_arg[1];
  retval_add_func add_function = NULL;
  retval_i32_func offset_function = NULL;
  retval_i32_func add_one_function_ptr = NULL;
  retval_f64_func f64_function = NULL;
  retval_void_func void_function = NULL;
  retval_string_factory_func string_function = NULL;
  retval_string_factory_func null_string_function = NULL;
  retval_buffer_view_factory_func buffer_view_function = NULL;
  retval_function_factory_func function_factory = NULL;
  retval_function_arg_func function_arg = NULL;
  retval_i32_func raw_rejected = NULL;
  i32_func completion_add_one = NULL;
  union {
    i32_func completion;
    retval_i32_func retval;
  } mismatched_function;
  int32_t offset = 10;
  int32_t result = 0;
  int passed = 1;

  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }

  passed = expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_retval_sig_add_i32,
                           retval_add_i32_function, &add_function, &error),
                       error.message) &&
           passed;
  if (add_function != NULL) {
    passed = expect_true(add_function(40, 2) == 42,
                         "retval pure function result mismatch") &&
             passed;
  }

  passed = expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_retval_sig_echo_i32,
                           retval_closure_add_state_function, &offset, NULL,
                           &offset_function, &error),
                       error.message) &&
           passed;
  if (offset_function != NULL) {
    passed = expect_true(offset_function(32) == 42,
                         "retval closure result mismatch") &&
             passed;
  }

  passed = expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_retval_sig_echo_f64,
                           retval_echo_f64_function, &f64_function, &error),
                       error.message) &&
           passed;
  if (f64_function != NULL) {
    passed = expect_true(isnan(f64_function(NAN)),
                         "retval double nan was not preserved") &&
             passed;
    passed = expect_true(is_positive_infinity_double(f64_function(INFINITY)),
                         "retval double infinity was not preserved") &&
             passed;
  }

  g_retval_void_call_count = 0;
  passed = expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_retval_sig_void,
                           retval_void_function, &void_function, &error),
                       error.message) &&
           passed;
  if (void_function != NULL) {
    void_function();
    passed = expect_true(g_retval_void_call_count == 1,
                         "retval void function was not called") &&
             passed;
  }

  passed = expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_retval_sig_return_string,
                           retval_return_string_function, &string_function,
                           &error),
                       error.message) &&
           passed;
  if (string_function != NULL) {
    passed = expect_true(string_function() == k_retval_borrowed_string,
                         "retval string was not borrowed") &&
             passed;
  }

  passed = expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_retval_sig_return_string,
                           retval_return_null_string_function,
                           &null_string_function, &error),
                       error.message) &&
           passed;
  if (null_string_function != NULL) {
    passed = expect_true(null_string_function() == NULL,
                         "retval null string was not preserved") &&
             passed;
  }

  passed = expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_retval_sig_return_buffer_view,
                           retval_return_buffer_view_function,
                           &buffer_view_function, &error),
                       error.message) &&
           passed;
  if (buffer_view_function != NULL) {
    tra_ffic_buffer_view view = buffer_view_function();
    passed = expect_true(view.data == k_retval_buffer_data &&
                             view.size == sizeof(k_retval_buffer_data),
                         "retval buffer view mismatch") &&
             passed;
  }

  memset(&capture, 0, sizeof(capture));
  memset(&target_ref, 0, sizeof(target_ref));
  add_args[0] = tra_ffic_value_int32(20);
  add_args[1] = tra_ffic_value_int32(22);
  target_ref.raw = (tra_ffic_native_function)add_function;
  target_ref.owner_side = &context.side_b;
  target_ref.signature = &k_retval_sig_add_i32;
  passed = expect_true(tra_ffic_call_with_result(
                           &context.side_a, &target_ref, add_args, 2u,
                           capture_result_callback, &capture, &error),
                       error.message) &&
           passed;
  passed = expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 42,
                       "retval structured result mismatch") &&
           passed;

  memset(&capture, 0, sizeof(capture));
  memset(&target_ref, 0, sizeof(target_ref));
  one_arg[0] = tra_ffic_value_int32(32);
  target_ref.raw = (tra_ffic_native_function)offset_function;
  target_ref.owner_side = &context.side_b;
  target_ref.signature = &k_retval_sig_echo_i32;
  passed = expect_true(tra_ffic_call(&context.side_a, &target_ref, one_arg, 1u,
                                     capture_i32_success_callback, &capture,
                                     &error),
                       error.message) &&
           passed;
  passed = expect_true(capture.count == 1 && capture.int32_value == 42,
                       "retval success call result mismatch") &&
           passed;

  memset(&capture, 0, sizeof(capture));
  memset(&target_ref, 0, sizeof(target_ref));
  target_ref.raw = (tra_ffic_native_function)string_function;
  target_ref.owner_side = &context.side_b;
  target_ref.signature = &k_retval_sig_return_string;
  passed = expect_true(tra_ffic_call_with_result(
                           &context.side_a, &target_ref, NULL, 0u,
                           capture_result_callback, &capture, &error),
                       error.message) &&
           passed;
  passed = expect_true(capture.count == 1 && !capture.has_error &&
                           capture.string_pointer == k_retval_borrowed_string &&
                           strcmp(capture.string_value,
                                  k_retval_borrowed_string) == 0,
                       "retval structured string mismatch") &&
           passed;

  memset(&capture, 0, sizeof(capture));
  memset(&target_ref, 0, sizeof(target_ref));
  target_ref.raw = (tra_ffic_native_function)null_string_function;
  target_ref.owner_side = &context.side_b;
  target_ref.signature = &k_retval_sig_return_string;
  passed = expect_true(tra_ffic_call_with_result(
                           &context.side_a, &target_ref, NULL, 0u,
                           capture_result_callback, &capture, &error),
                       error.message) &&
           passed;
  passed = expect_true(capture.count == 1 && !capture.has_error &&
                           capture.string_was_null,
                       "retval structured null string mismatch") &&
           passed;

  memset(&capture, 0, sizeof(capture));
  memset(&target_ref, 0, sizeof(target_ref));
  target_ref.raw = (tra_ffic_native_function)buffer_view_function;
  target_ref.owner_side = &context.side_b;
  target_ref.signature = &k_retval_sig_return_buffer_view;
  passed = expect_true(tra_ffic_call_with_result(
                           &context.side_a, &target_ref, NULL, 0u,
                           capture_result_callback, &capture, &error),
                       error.message) &&
           passed;
  passed = expect_true(capture.count == 1 && !capture.has_error &&
                           capture.buffer_view_value.data ==
                               k_retval_buffer_data &&
                           capture.buffer_view_value.size ==
                               sizeof(k_retval_buffer_data),
                       "retval structured buffer view mismatch") &&
           passed;

  passed = expect_true(tra_ffic_side_create_pure_function(
                           &context.side_a, &k_retval_sig_echo_i32,
                           retval_add_one_function, &add_one_function_ptr,
                           &error),
                       error.message) &&
           passed;
  passed = expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_retval_sig_return_function,
                           retval_return_function_closure,
                           &add_one_function_ptr, NULL, &function_factory,
                           &error),
                       error.message) &&
           passed;
  memset(&capture, 0, sizeof(capture));
  memset(&target_ref, 0, sizeof(target_ref));
  target_ref.raw = (tra_ffic_native_function)function_factory;
  target_ref.owner_side = &context.side_b;
  target_ref.signature = &k_retval_sig_return_function;
  passed = expect_true(tra_ffic_call_with_result(
                           &context.side_a, &target_ref, NULL, 0u,
                           capture_result_callback, &capture, &error),
                       error.message) &&
           passed;
  if (capture.native_function_value != NULL) {
    retval_i32_func returned =
        (retval_i32_func)capture.native_function_value;
    passed = expect_true(capture.count == 1 && !capture.has_error &&
                             returned(41) == 42,
                         "retval structured function return mismatch") &&
             passed;
  } else {
    passed = expect_true(0, "retval structured function return was null") &&
             passed;
  }

  passed = expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_retval_sig_function_arg,
                           retval_accept_function, &function_arg, &error),
                       error.message) &&
           passed;
  if (function_arg != NULL && add_one_function_ptr != NULL) {
    result = function_arg(add_one_function_ptr);
    passed = expect_true(result == 11,
                         "retval function argument result mismatch") &&
             passed;
  }
  passed = expect_true(tra_ffic_side_create_pure_function(
                           &context.side_a, &k_sig_echo_i32, add_one_function,
                           &completion_add_one, &error),
                       error.message) &&
           passed;
  if (function_arg != NULL && completion_add_one != NULL) {
    mismatched_function.completion = completion_add_one;
    result = function_arg(mismatched_function.retval);
    passed = expect_true(result == 0,
                         "retval function argument ABI mismatch was accepted") &&
             passed;
  }

  passed = expect_true(!tra_ffic_side_create_raw_closure(
                           &context.side_b, &k_retval_sig_echo_i32,
                           accept_function_raw, NULL, NULL, &raw_rejected,
                           &error),
                       "retval raw closure unexpectedly succeeded") &&
           passed;
  passed = expect_true(strstr(error.message, "retval ABI") != NULL,
                       "retval raw closure error message mismatch") &&
           passed;

  if (completion_add_one != NULL) {
    passed = expect_true(tra_ffic_function_release(completion_add_one, &error),
                         error.message) &&
             passed;
  }
  if (function_arg != NULL) {
    passed = expect_true(tra_ffic_function_release(function_arg, &error),
                         error.message) &&
             passed;
  }
  if (function_factory != NULL) {
    passed = expect_true(tra_ffic_function_release(function_factory, &error),
                         error.message) &&
             passed;
  }
  if (add_one_function_ptr != NULL) {
    passed = expect_true(tra_ffic_function_release(add_one_function_ptr, &error),
                         error.message) &&
             passed;
  }
  if (buffer_view_function != NULL) {
    passed = expect_true(tra_ffic_function_release(buffer_view_function, &error),
                         error.message) &&
             passed;
  }
  if (null_string_function != NULL) {
    passed = expect_true(tra_ffic_function_release(null_string_function, &error),
                         error.message) &&
             passed;
  }
  if (string_function != NULL) {
    passed = expect_true(tra_ffic_function_release(string_function, &error),
                         error.message) &&
             passed;
  }
  if (void_function != NULL) {
    passed = expect_true(tra_ffic_function_release(void_function, &error),
                         error.message) &&
             passed;
  }
  if (f64_function != NULL) {
    passed = expect_true(tra_ffic_function_release(f64_function, &error),
                         error.message) &&
             passed;
  }
  if (offset_function != NULL) {
    passed = expect_true(tra_ffic_function_release(offset_function, &error),
                         error.message) &&
             passed;
  }
  if (add_function != NULL) {
    passed = expect_true(tra_ffic_function_release(add_function, &error),
                         error.message) &&
             passed;
  }

  passed = test_context_drain(&context) && passed;
  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_buffer_view_test(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  tra_ffic_completion completion = NULL;
  typed_capture capture;
  typed_capture raw_capture;
  buffer_view_func buffer_function = NULL;
  void_func invalid_function = NULL;
  uint8_t bytes[4];
  tra_ffic_buffer_view view;
  tra_ffic_type helper_type;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }

  helper_type = tra_ffic_type_buffer_view();
  passed = passed &&
           expect_true(helper_type.kind == TRA_FFIC_TYPE_BUFFER_VIEW &&
                           helper_type.function_signature == NULL,
                       "buffer view type helper mismatch");

  memset(bytes, 0, sizeof(bytes));
  bytes[0] = 1u;
  bytes[1] = 2u;
  bytes[2] = 3u;
  bytes[3] = 4u;
  view.data = bytes;
  view.size = sizeof(bytes);
  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_sig_echo_buffer_view,
                           echo_buffer_view_function, &buffer_function,
                           &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_buffer_view,
                           capture_buffer_view_callback, &completion,
                           &capture, &error),
                       error.message);
  buffer_function(completion, view);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.buffer_view_value.data == bytes &&
                           capture.buffer_view_value.size == sizeof(bytes) &&
                           bytes[1] == 0x42u,
                       "pure buffer view result mismatch");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(buffer_function, &error),
                       error.message);
  completion = NULL;
  buffer_function = NULL;
  passed = test_context_drain(&context) && passed;

  memset(bytes, 0, sizeof(bytes));
  view.data = bytes;
  view.size = sizeof(bytes);
  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_sig_echo_buffer_view,
                           closure_echo_buffer_view_function, NULL, NULL,
                           &buffer_function, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_buffer_view,
                           capture_buffer_view_callback, &completion,
                           &capture, &error),
                       error.message);
  buffer_function(completion, view);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.buffer_view_value.data == bytes &&
                           capture.buffer_view_value.size == sizeof(bytes) &&
                           bytes[2] == 0x24u,
                       "closure buffer view result mismatch");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(buffer_function, &error),
                       error.message);
  completion = NULL;
  buffer_function = NULL;
  passed = test_context_drain(&context) && passed;

  memset(bytes, 0, sizeof(bytes));
  view.data = bytes;
  view.size = sizeof(bytes);
  memset(&capture, 0, sizeof(capture));
  memset(&raw_capture, 0, sizeof(raw_capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_raw_closure(
                           &context.side_b, &k_sig_echo_buffer_view,
                           accept_buffer_view_raw, &raw_capture, NULL,
                           &buffer_function, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_buffer_view,
                           capture_buffer_view_callback, &completion,
                           &capture, &error),
                       error.message);
  buffer_function(completion, view);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           raw_capture.count == 1 &&
                           !raw_capture.has_error &&
                           raw_capture.buffer_view_value.data == bytes &&
                           capture.buffer_view_value.data == bytes &&
                           capture.buffer_view_value.size == sizeof(bytes) &&
                           bytes[3] == 0x18u,
                       "raw buffer view result mismatch");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(buffer_function, &error),
                       error.message);
  completion = NULL;
  buffer_function = NULL;
  passed = test_context_drain(&context) && passed;

  memset(bytes, 0, sizeof(bytes));
  view.data = bytes;
  view.size = sizeof(bytes);
  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_sig_echo_buffer_view,
                           closure_echo_buffer_view_function, NULL, NULL,
                           &buffer_function, &error),
                       error.message);
  {
    tra_ffic_function_ref target_ref;
    tra_ffic_value arg_value =
        tra_ffic_value_buffer_view(view.data, view.size);
    memset(&target_ref, 0, sizeof(target_ref));
    target_ref.raw = (tra_ffic_native_function)buffer_function;
    target_ref.owner_side = &context.side_b;
    target_ref.signature = &k_sig_echo_buffer_view;
    passed = passed &&
             expect_true(tra_ffic_call_with_result(
                             &context.side_a, &target_ref, &arg_value, 1u,
                             capture_result_callback, &capture, &error),
                         error.message);
  }
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.buffer_view_value.data == bytes &&
                           capture.buffer_view_value.size == sizeof(bytes) &&
                           bytes[2] == 0x24u,
                       "structured buffer view result mismatch");

  memset(&capture, 0, sizeof(capture));
  {
    tra_ffic_function_ref target_ref;
    tra_ffic_value arg_value = tra_ffic_value_buffer_view(NULL, 0u);
    memset(&target_ref, 0, sizeof(target_ref));
    target_ref.raw = (tra_ffic_native_function)buffer_function;
    target_ref.owner_side = &context.side_b;
    target_ref.signature = &k_sig_echo_buffer_view;
    passed = passed &&
             expect_true(tra_ffic_call_with_result(
                             &context.side_a, &target_ref, &arg_value, 1u,
                             capture_result_callback, &capture, &error),
                         error.message);
  }
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.buffer_view_value.data == NULL &&
                           capture.buffer_view_value.size == 0u,
                       "empty buffer view result mismatch");

  memset(&capture, 0, sizeof(capture));
  {
    tra_ffic_function_ref target_ref;
    tra_ffic_value arg_value = tra_ffic_value_buffer_view(NULL, 1u);
    memset(&target_ref, 0, sizeof(target_ref));
    target_ref.raw = (tra_ffic_native_function)buffer_function;
    target_ref.owner_side = &context.side_b;
    target_ref.signature = &k_sig_echo_buffer_view;
    passed = passed &&
             expect_true(!tra_ffic_call_with_result(
                             &context.side_a, &target_ref, &arg_value, 1u,
                             capture_result_callback, &capture, &error),
                         "invalid buffer view argument was accepted");
    passed = passed &&
             expect_true(strcmp(error.message, "Invalid buffer view") == 0,
                         "invalid buffer view argument error mismatch");
  }

  memset(&capture, 0, sizeof(capture));
  view.data = NULL;
  view.size = 1u;
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_buffer_view,
                           capture_buffer_view_callback, &completion,
                           &capture, &error),
                       error.message);
  buffer_function(completion, view);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && capture.has_error &&
                           strcmp(capture.error_message,
                                  "Invalid buffer view") == 0,
                       "invalid closure buffer view error mismatch");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  completion = NULL;
  passed = test_context_drain(&context) && passed;

  memset(bytes, 0, sizeof(bytes));
  view.data = bytes;
  view.size = sizeof(bytes);
  memset(&capture, 0, sizeof(capture));
  {
    tra_ffic_function_ref target_ref;
    tra_ffic_value arg_value =
        tra_ffic_value_buffer_view(view.data, view.size);
    memset(&target_ref, 0, sizeof(target_ref));
    target_ref.raw = (tra_ffic_native_function)buffer_function;
    target_ref.owner_side = &context.side_b;
    target_ref.signature = &k_sig_echo_buffer_view;
    passed = passed &&
             expect_true(tra_ffic_call(
                             &context.side_a, &target_ref, &arg_value, 1u,
                             capture_buffer_view_success_callback, &capture,
                             &error),
                         error.message);
  }
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 &&
                           capture.buffer_view_value.data == bytes &&
                           capture.buffer_view_value.size == sizeof(bytes) &&
                           bytes[2] == 0x24u,
                       "success buffer view result mismatch");
  passed = passed &&
           expect_true(tra_ffic_function_release(buffer_function, &error),
                       error.message);
  buffer_function = NULL;
  passed = test_context_drain(&context) && passed;

  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_sig_return_buffer_view,
                           return_invalid_buffer_view_function,
                           &invalid_function, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_buffer_view,
                           capture_buffer_view_callback, &completion,
                           &capture, &error),
                       error.message);
  invalid_function(completion);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && capture.has_error &&
                           strcmp(capture.error_message,
                                  "Invalid buffer view") == 0 &&
                           capture.buffer_view_value.data == NULL &&
                           capture.buffer_view_value.size == 0u,
                       "invalid buffer view completion mismatch");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(invalid_function, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;

  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_completion_behavior_test(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  void_func function = NULL;
  tra_ffic_completion completion = NULL;
  typed_capture capture;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }

  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_sig_return_string,
                           return_stack_string_function, &function, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_string,
                           capture_string_callback, &completion, &capture,
                           &error),
                       error.message);
  function(completion);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           strcmp(capture.string_value, "copied-string") == 0,
                       "completion did not copy string result");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(function, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;

  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_sig_return_string,
                           return_null_string_function, &function, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_string,
                           capture_string_callback, &completion, &capture,
                           &error),
                       error.message);
  function(completion);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.string_was_null,
                       "completion did not preserve null string result");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(function, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;

  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_sig_return_string,
                           return_missing_string_function, &function, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_string,
                           capture_string_callback, &completion, &capture,
                           &error),
                       error.message);
  function(completion);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && capture.has_error &&
                           strcmp(capture.error_message,
                                  "Completion result is null") == 0,
                       "null completion result error mismatch");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(function, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;

  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_sig_return_f64,
                           return_nan_function, &function, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_f64, capture_f64_callback,
                           &completion, &capture, &error),
                       error.message);
  function(completion);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           isnan(capture.double_value),
                       "nan double completion was not preserved");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(function, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;

  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_sig_return_f64,
                           return_infinity_function, &function, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_f64, capture_f64_callback,
                           &completion, &capture, &error),
                       error.message);
  function(completion);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           is_positive_infinity_double(capture.double_value),
                       "infinite double completion was not preserved");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(function, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;

  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_sig_return_f32,
                           return_nan_float_function, &function, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_f32, capture_f32_callback,
                           &completion, &capture, &error),
                       error.message);
  function(completion);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           isnan(capture.float_value),
                       "nan float completion was not preserved");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(function, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;

  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_sig_return_f32,
                           return_infinity_float_function, &function, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_f32, capture_f32_callback,
                           &completion, &capture, &error),
                       error.message);
  function(completion);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           is_positive_infinity_float(capture.float_value),
                       "infinite float completion was not preserved");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(function, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;

  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_sig_return_i32,
                           fail_i32_function, &function, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32, capture_i32_callback,
                           &completion, &capture, &error),
                       error.message);
  function(completion);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && capture.has_error &&
                           strcmp(capture.error_message,
                                  "expected failure") == 0,
                       "completion error was not delivered");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(function, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;

  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_sig_return_string,
                           double_completion_function, &function, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_string,
                           capture_string_callback, &completion, &capture,
                           &error),
                       error.message);
  function(completion);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           strcmp(capture.string_value, "first") == 0,
                       "double completion did not preserve first result");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(function, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;

  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_async_completion_test(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  i32_func function = NULL;
  tra_ffic_completion completion = NULL;
  typed_capture capture;
  async_state state;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }
  memset(&capture, 0, sizeof(capture));
  memset(&state, 0, sizeof(state));
  passed = passed &&
           expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_sig_echo_i32, async_function,
                           &state, NULL, &function, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32, capture_i32_callback,
                           &completion, &capture, &error),
                       error.message);
  function(completion, 41);
  passed = passed && expect_true(capture.count == 0,
                                 "async completion delivered inline");
  if (state.thread_started) {
#if TRA_FFIC_IN_WIN32
    (void)WaitForSingleObject(state.thread, INFINITE);
    CloseHandle(state.thread);
    state.thread = NULL;
#else
    pthread_join(state.thread, NULL);
#endif
  }
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 42,
                       "async completion result mismatch");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(function, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;
  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_pointer_list_argument_passing_test(
    test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  typed_capture capture;
  add_func stack_to_args = NULL;
  add_func stack_to_args_closure = NULL;
  args_add_func args_to_stack = NULL;
  retval_add_func retval_stack_to_args = NULL;
  retval_args_add_func retval_args_to_stack = NULL;
  i32_func add_one = NULL;
  args_function_arg_func accept_args_function = NULL;
  function_factory_func args_factory = NULL;
  args_i32_func returned_args_function = NULL;
  tra_ffic_completion completion = NULL;
  int32_t first = 19;
  int32_t second = 23;
  int32_t value = 41;
  int32_t offset = 10;
  const void *two_args[2];
  const void *one_arg[1];
  nested_callback_state nested_state;
  union {
    i32_func stack;
    args_i32_func args;
  } adapted_function;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }
  two_args[0] = &first;
  two_args[1] = &second;
  one_arg[0] = &value;

  passed = passed &&
           expect_true(tra_ffic_side_create_pure_pointer_list_function(
                           &context.side_b, &k_sig_add_i32,
                           add_i32_pointer_list_function, &stack_to_args,
                           &error),
                       error.message);
  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32,
                           capture_i32_callback, &completion, &capture,
                           &error),
                       error.message);
  stack_to_args(completion, first, second);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 42,
                       "stack caller to pointer-list callee failed");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  completion = NULL;

  passed = passed &&
           expect_true(tra_ffic_side_create_pointer_list_closure(
                           &context.side_b, &k_sig_add_i32,
                           add_i32_pointer_list_closure, &offset, NULL,
                           &stack_to_args_closure, &error),
                       error.message);
  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32,
                           capture_i32_callback, &completion, &capture,
                           &error),
                       error.message);
  stack_to_args_closure(completion, 10, 22);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 42,
                       "stack caller to pointer-list closure failed");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  completion = NULL;

  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_sig_args_add_i32,
                           add_i32_function, &args_to_stack, &error),
                       error.message);
  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32,
                           capture_i32_callback, &completion, &capture,
                           &error),
                       error.message);
  args_to_stack(completion, two_args);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 42,
                       "pointer-list caller to stack callee failed");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  completion = NULL;

  passed = passed &&
           expect_true(tra_ffic_side_create_pure_pointer_list_function(
                           &context.side_b, &k_retval_sig_add_i32,
                           retval_add_i32_pointer_list_function,
                           &retval_stack_to_args, &error),
                       error.message);
  if (retval_stack_to_args != NULL) {
    passed = passed &&
             expect_true(retval_stack_to_args(first, second) == 42,
                         "retval stack caller to pointer-list callee failed");
  }

  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_retval_sig_args_add_i32,
                           retval_add_i32_function, &retval_args_to_stack,
                           &error),
                       error.message);
  if (retval_args_to_stack != NULL) {
    passed = passed &&
             expect_true(retval_args_to_stack(two_args) == 42,
                         "retval pointer-list caller to stack callee failed");
  }

  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_a, &k_sig_echo_i32,
                           add_one_function, &add_one, &error),
                       error.message);
  nested_state.caller_side = &context.side_b;
  nested_state.queue = &context.queue;
  nested_state.drain_mode = drain_mode;
  passed = passed &&
           expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_sig_args_function_arg,
                           call_args_function, &nested_state, NULL,
                           &accept_args_function, &error),
                       error.message);
  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32,
                           capture_i32_callback, &completion, &capture,
                           &error),
                       error.message);
  adapted_function.stack = add_one;
  accept_args_function(completion, adapted_function.args);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 42,
                       "pointer-list function argument adapter failed");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  completion = NULL;

  passed = passed &&
           expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_sig_return_args_function,
                           return_function, &add_one, NULL, &args_factory,
                           &error),
                       error.message);
  memset(&capture, 0, sizeof(capture));
  capture.retain_function = 1;
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_args_i32_function,
                           capture_args_function_callback, &completion,
                           &capture, &error),
                       error.message);
  args_factory(completion);
  passed = test_context_drain(&context) && passed;
  returned_args_function = (args_i32_func)capture.native_function_value;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           returned_args_function != NULL,
                       "pointer-list function return adapter missing");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  completion = NULL;

  if (returned_args_function != NULL) {
    memset(&capture, 0, sizeof(capture));
    passed = passed &&
             expect_true(tra_ffic_side_create_completion_function(
                             &context.side_a, &k_type_i32,
                             capture_i32_callback, &completion, &capture,
                             &error),
                         error.message);
    returned_args_function(completion, one_arg);
    passed = test_context_drain(&context) && passed;
    passed = passed &&
             expect_true(capture.count == 1 && !capture.has_error &&
                             capture.int32_value == 42,
                         "returned pointer-list function call failed");
    passed = passed &&
             expect_true(tra_ffic_function_release(completion, &error),
                         error.message);
    completion = NULL;
  }

  if (completion != NULL) {
    passed = expect_true(tra_ffic_function_release(completion, &error),
                         error.message) &&
             passed;
  }
  if (returned_args_function != NULL) {
    passed = expect_true(tra_ffic_function_release(returned_args_function,
                                                   &error),
                         error.message) &&
             passed;
  }
  if (args_factory != NULL) {
    passed = expect_true(tra_ffic_function_release(args_factory, &error),
                         error.message) &&
             passed;
  }
  if (accept_args_function != NULL) {
    passed = expect_true(tra_ffic_function_release(accept_args_function,
                                                   &error),
                         error.message) &&
             passed;
  }
  if (add_one != NULL) {
    passed = expect_true(tra_ffic_function_release(add_one, &error),
                         error.message) &&
             passed;
  }
  if (retval_args_to_stack != NULL) {
    passed = expect_true(tra_ffic_function_release(retval_args_to_stack,
                                                   &error),
                         error.message) &&
             passed;
  }
  if (retval_stack_to_args != NULL) {
    passed = expect_true(tra_ffic_function_release(retval_stack_to_args,
                                                   &error),
                         error.message) &&
             passed;
  }
  if (args_to_stack != NULL) {
    passed = expect_true(tra_ffic_function_release(args_to_stack, &error),
                         error.message) &&
             passed;
  }
  if (stack_to_args_closure != NULL) {
    passed = expect_true(tra_ffic_function_release(stack_to_args_closure,
                                                   &error),
                         error.message) &&
             passed;
  }
  if (stack_to_args != NULL) {
    passed = expect_true(tra_ffic_function_release(stack_to_args, &error),
                         error.message) &&
             passed;
  }
  passed = test_context_drain(&context) && passed;
  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_function_marshalling_test(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  i32_func add_one = NULL;
  function_arg_func nested = NULL;
  function_arg_func pure_accept = NULL;
  function_arg_func raw_accept = NULL;
  function_factory_func factory = NULL;
  function_factory_func null_factory = NULL;
  i32_func null_function = NULL;
  i32_func returned_function = NULL;
  tra_ffic_completion completion = NULL;
  typed_capture capture;
  typed_capture raw_capture;
  nested_callback_state nested_state;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }

  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_a, &k_sig_echo_i32, add_one_function,
                           &add_one, &error),
                       error.message);
  passed = passed &&
           expect_true(add_one == add_one_function,
                       "non-nested pure function was not returned directly");
  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_sig_function_arg,
                           accept_function_pure, &pure_accept, &error),
                       error.message);
  passed = passed &&
           expect_true(pure_accept != accept_function_pure,
                       "function-typed pure function was not wrapped");
  memset(&capture, 0, sizeof(capture));
  g_seen_function_argument = NULL;
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32, capture_i32_callback,
                           &completion, &capture, &error),
                       error.message);
  pure_accept(completion, add_one);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 1 &&
                           g_seen_function_argument == add_one,
                       "pure function argument was not passed as raw pointer");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);

  memset(&capture, 0, sizeof(capture));
  g_seen_function_argument = add_one;
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32, capture_i32_callback,
                           &completion, &capture, &error),
                       error.message);
  pure_accept(completion, NULL);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 1 &&
                           g_seen_function_argument == NULL,
                       "null function argument was not passed to callback");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);

  {
    tra_ffic_function_ref target_ref;
    tra_ffic_value arg_value = tra_ffic_value_function(NULL);
    memset(&capture, 0, sizeof(capture));
    memset(&target_ref, 0, sizeof(target_ref));
    g_seen_function_argument = add_one;
    target_ref.raw = (tra_ffic_native_function)pure_accept;
    target_ref.owner_side = &context.side_b;
    target_ref.signature = &k_sig_function_arg;
    passed = passed &&
             expect_true(tra_ffic_call_with_result(
                             &context.side_a, &target_ref, &arg_value, 1u,
                             capture_result_callback, &capture, &error),
                         error.message);
    passed = test_context_drain(&context) && passed;
    passed = passed &&
             expect_true(capture.count == 1 && !capture.has_error &&
                             capture.int32_value == 1 &&
                             g_seen_function_argument == NULL,
                         "structured null function argument failed");
  }

  memset(&capture, 0, sizeof(capture));
  memset(&raw_capture, 0, sizeof(raw_capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_raw_closure(
                           &context.side_b, &k_sig_function_arg,
                           accept_function_raw, &raw_capture, NULL,
                           &raw_accept, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32, capture_i32_callback,
                           &completion, &capture, &error),
                       error.message);
  raw_accept(completion, NULL);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 7 &&
                           raw_capture.count == 1 && !raw_capture.has_error &&
                           raw_capture.function_value == NULL,
                       "raw null function argument failed");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);

  nested_state.caller_side = &context.side_b;
  nested_state.queue = &context.queue;
  nested_state.drain_mode = drain_mode;
  passed = passed &&
           expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_sig_function_arg,
                           call_nested_function, &nested_state, NULL, &nested,
                           &error),
                       error.message);
  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32, capture_i32_callback,
                           &completion, &capture, &error),
                       error.message);
  nested(completion, add_one);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 42,
                       "nested function argument call failed");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);

  passed = passed &&
           expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_sig_return_function,
                           return_function, &add_one, NULL, &factory, &error),
                       error.message);
  memset(&capture, 0, sizeof(capture));
  capture.retain_function = 1;
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32_function,
                           capture_function_callback, &completion, &capture,
                           &error),
                       error.message);
  factory(completion);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.function_value == add_one,
                       "function return did not preserve function pointer");
  returned_function = capture.function_value;
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);

  passed = passed &&
           expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_sig_return_function,
                           return_function, &null_function, NULL,
                           &null_factory, &error),
                       error.message);
  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32_function,
                           capture_function_callback, &completion, &capture,
                           &error),
                       error.message);
  null_factory(completion);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.function_value == NULL,
                       "null function return failed");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);

  {
    tra_ffic_function_ref target_ref;
    memset(&capture, 0, sizeof(capture));
    memset(&target_ref, 0, sizeof(target_ref));
    target_ref.raw = (tra_ffic_native_function)null_factory;
    target_ref.owner_side = &context.side_b;
    target_ref.signature = &k_sig_return_function;
    passed = passed &&
             expect_true(tra_ffic_call_with_result(
                             &context.side_a, &target_ref, NULL, 0u,
                             capture_result_callback, &capture, &error),
                         error.message);
    passed = test_context_drain(&context) && passed;
    passed = passed &&
             expect_true(capture.count == 1 && !capture.has_error &&
                             capture.function_value == NULL,
                         "structured null function return failed");
  }

  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32, capture_i32_callback,
                           &completion, &capture, &error),
                       error.message);
  returned_function(completion, 10);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 11,
                       "returned function call failed");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);

  passed = passed &&
           expect_true(tra_ffic_function_release(add_one, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(returned_function, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(nested, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(factory, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(null_factory, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(raw_accept, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(pure_accept, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;
  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_function_identity_test(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  i32_func pure_first = NULL;
  i32_func pure_second = NULL;
  i32_func add_one = NULL;
  function_arg_func wrapped_first = NULL;
  function_arg_func wrapped_second = NULL;
  tra_ffic_completion completion = NULL;
  typed_capture capture;
  i32_func closure_first = NULL;
  i32_func closure_second = NULL;
  i32_func closure_other_state = NULL;
  i32_func closure_other_finalizer = NULL;
  finalize_counter finalizer;
  finalize_counter other_state_finalizer;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }

  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_a, &k_sig_echo_i32,
                           echo_i32_function, &pure_first, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_a, &k_sig_echo_i32,
                           echo_i32_function, &pure_second, &error),
                       error.message);
  passed = passed &&
           expect_true(pure_first == echo_i32_function &&
                           pure_second == echo_i32_function &&
                           pure_first == pure_second,
                       "pure primitive identity was not preserved");
  passed = passed &&
           expect_true(tra_ffic_function_release(pure_first, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(tra_ffic_function_release(pure_second, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(!tra_ffic_function_release(pure_first, &error),
                       "extra pure primitive release unexpectedly succeeded");

  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_a, &k_sig_echo_i32, add_one_function,
                           &add_one, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_sig_function_arg,
                           accept_function_pure, &wrapped_first, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_pure_function(
                           &context.side_b, &k_sig_function_arg,
                           accept_function_pure, &wrapped_second, &error),
                       error.message);
  passed = passed &&
           expect_true(wrapped_first == wrapped_second &&
                           wrapped_first != accept_function_pure,
                       "wrapped pure identity was not preserved");
  memset(&capture, 0, sizeof(capture));
  g_seen_function_argument = NULL;
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32, capture_i32_callback,
                           &completion, &capture, &error),
                       error.message);
  wrapped_first(completion, add_one);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 1 &&
                           g_seen_function_argument == add_one,
                       "wrapped pure identity call failed");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(add_one, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(wrapped_first, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(wrapped_second, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;

  memset(&finalizer, 0, sizeof(finalizer));
  memset(&other_state_finalizer, 0, sizeof(other_state_finalizer));
  passed = passed &&
           expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_sig_echo_i32,
                           closure_echo_i32_function, &finalizer,
                           finalize_counter_callback, &closure_first, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_sig_echo_i32,
                           closure_echo_i32_function, &finalizer,
                           finalize_counter_callback, &closure_second, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_sig_echo_i32,
                           closure_echo_i32_function, &other_state_finalizer,
                           finalize_counter_callback, &closure_other_state,
                           &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_sig_echo_i32,
                           closure_echo_i32_function, &finalizer,
                           finalize_counter_alt_callback,
                           &closure_other_finalizer, &error),
                       error.message);
  passed = passed &&
           expect_true(closure_first == closure_second,
                       "closure identity was not preserved");
  passed = passed &&
           expect_true(closure_other_state != closure_first,
                       "closure reused a different state");
  passed = passed &&
           expect_true(closure_other_finalizer != closure_first,
                       "closure reused a different finalizer");
  passed = passed &&
           expect_true(tra_ffic_function_release(closure_first, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;
  passed = passed && expect_true(finalizer.count == 0,
                                 "first shared closure release finalized");
  passed = passed &&
           expect_true(tra_ffic_function_release(closure_second, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;
  passed = passed && expect_true(finalizer.count == 1,
                                 "shared closure finalizer mismatch");
  passed = passed &&
           expect_true(tra_ffic_function_release(closure_other_state, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;
  passed = passed && expect_true(other_state_finalizer.count == 1,
                                 "different-state closure was not finalized");
  passed = passed &&
           expect_true(tra_ffic_function_release(closure_other_finalizer,
                                                 &error),
                       error.message);
  passed = test_context_drain(&context) && passed;
  passed = passed && expect_true(finalizer.count == 2,
                                 "different-finalizer closure mismatch");

  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_function_argument_error_test(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  function_arg_func nested = NULL;
  tra_ffic_completion completion = NULL;
  typed_capture capture;
  nested_callback_state nested_state;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }
  nested_state.caller_side = &context.side_b;
  nested_state.queue = &context.queue;
  nested_state.drain_mode = drain_mode;
  passed = passed &&
           expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_sig_function_arg,
                           call_nested_function, &nested_state, NULL, &nested,
                           &error),
                       error.message);
  memset(&capture, 0, sizeof(capture));
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32, capture_i32_callback,
                           &completion, &capture, &error),
                       error.message);
  nested(completion, add_one_function);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && capture.has_error &&
                           strcmp(capture.error_message,
                                  "Unknown function argument") == 0,
                       "unknown function argument was not rejected");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(nested, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;
  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_lifetime_test(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  i32_func closure = NULL;
  i32_func fake_function = echo_i32_function;
  tra_ffic_completion completion = NULL;
  finalize_counter finalizer;
  typed_capture capture;
  release_during_call_state release_state;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }

  memset(&finalizer, 0, sizeof(finalizer));
  passed = passed &&
           expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_sig_echo_i32,
                           closure_echo_i32_function, &finalizer,
                           finalize_counter_callback, &closure, &error),
                       error.message);
  passed = passed &&
           expect_true(closure != echo_i32_function,
                       "closure unexpectedly returned original function");
  passed = passed &&
           expect_true(tra_ffic_function_retain(closure, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_retain(closure, &error),
                       error.message);
  passed = passed &&
           expect_true(tra_ffic_function_release(closure, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;
  passed = passed && expect_true(finalizer.count == 0,
                                 "first release finalized retained closure");
  passed = passed &&
           expect_true(tra_ffic_function_release(closure, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;
  passed = passed && expect_true(finalizer.count == 0,
                                 "second release finalized retained closure");
  passed = passed &&
           expect_true(tra_ffic_function_release(closure, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;
  passed = passed && expect_true(finalizer.count == 1,
                                 "final release did not finalize closure");

  (void)snprintf(error.message, sizeof(error.message), "%s", "stale error");
  passed = passed &&
           expect_true(tra_ffic_function_retain(NULL, &error) &&
                           error.message[0] == '\0',
                       "retain did not ignore null");
  (void)snprintf(error.message, sizeof(error.message), "%s", "stale error");
  passed = passed &&
           expect_true(tra_ffic_function_release(NULL, &error) &&
                           error.message[0] == '\0',
                       "release did not ignore null");
  passed = passed &&
           expect_true(!tra_ffic_function_release(fake_function, &error),
                       "release unexpectedly accepted unknown function");

  memset(&finalizer, 0, sizeof(finalizer));
  memset(&capture, 0, sizeof(capture));
  memset(&release_state, 0, sizeof(release_state));
  release_state.finalizer = &finalizer;
  passed = passed &&
           expect_true(tra_ffic_side_create_closure(
                           &context.side_b, &k_sig_echo_i32,
                           release_during_call_function, &release_state,
                           finalize_release_during_call_callback, &closure,
                           &error),
                       error.message);
  release_state.self = closure;
  passed = passed &&
           expect_true(tra_ffic_side_create_completion_function(
                           &context.side_a, &k_type_i32, capture_i32_callback,
                           &completion, &capture, &error),
                       error.message);
  closure(completion, 4);
  passed = passed && expect_true(finalizer.count == 0,
                                 "closure finalized before trampoline returned");
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 5,
                       "release-during-call result mismatch");
  passed = passed && expect_true(finalizer.count == 1,
                                 "release-during-call did not finalize once");
  passed = passed &&
           expect_true(tra_ffic_function_release(completion, &error),
                       error.message);
  passed = test_context_drain(&context) && passed;

  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int expect_closure_tracker_balanced(
    const tra_ffic_closure_tracker_snapshot *before,
    const char *case_name) {
  const tra_ffic_closure_tracker_snapshot after =
      tra_ffic_get_closure_tracker_snapshot();
  char message[160];
  int passed = 1;
  (void)snprintf(message, sizeof(message),
                 "%s closure tracker did not observe allocations", case_name);
  passed = passed &&
           expect_true(after.alloc_count > before->alloc_count, message);
  (void)snprintf(message, sizeof(message),
                 "%s closure tracker alloc/free mismatch", case_name);
  passed = passed &&
           expect_true(after.alloc_count - before->alloc_count ==
                           after.free_count - before->free_count,
                       message);
  (void)snprintf(message, sizeof(message),
                 "%s closure tracker live count did not return", case_name);
  passed = passed &&
           expect_true(after.live_count == before->live_count, message);
  return passed;
}

static int run_leak_case(const char *case_name,
                         leak_case_function function,
                         test_drain_mode drain_mode) {
  const tra_ffic_closure_tracker_snapshot before =
      tra_ffic_get_closure_tracker_snapshot();
  const int case_passed = function(drain_mode);
  const int tracker_passed =
      expect_closure_tracker_balanced(&before, case_name);
  return case_passed && tracker_passed;
}

static int run_leak_normal_operation_case(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  i32_func pure = NULL;
  i32_func closure = NULL;
  tra_ffic_completion completion = NULL;
  typed_capture capture;
  finalize_counter finalizer;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }
  memset(&finalizer, 0, sizeof(finalizer));

  if (!expect_true(tra_ffic_side_create_pure_function(
                       &context.side_b, &k_sig_echo_i32, echo_i32_function,
                       &pure, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  if (!expect_true(tra_ffic_side_create_closure(
                       &context.side_b, &k_sig_echo_i32,
                       closure_echo_i32_function, &finalizer,
                       finalize_counter_callback, &closure, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }

  memset(&capture, 0, sizeof(capture));
  if (!expect_true(tra_ffic_side_create_completion_function(
                       &context.side_a, &k_type_i32, capture_i32_callback,
                       &completion, &capture, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  pure(completion, 12);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 12,
                       "leak normal pure call result mismatch");
  passed = expect_true(tra_ffic_function_release(completion, &error),
                       error.message) &&
           passed;
  completion = NULL;

  memset(&capture, 0, sizeof(capture));
  if (!expect_true(tra_ffic_side_create_completion_function(
                       &context.side_a, &k_type_i32, capture_i32_callback,
                       &completion, &capture, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  closure(completion, 34);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 34,
                       "leak normal closure call result mismatch");
  passed = expect_true(tra_ffic_function_release(completion, &error),
                       error.message) &&
           passed;
  completion = NULL;

cleanup:
  if (completion != NULL) {
    passed = expect_true(tra_ffic_function_release(completion, &error),
                         error.message) &&
             passed;
  }
  if (closure != NULL) {
    passed = expect_true(tra_ffic_function_release(closure, &error),
                         error.message) &&
             passed;
  }
  if (pure != NULL) {
    passed = expect_true(tra_ffic_function_release(pure, &error),
                         error.message) &&
             passed;
  }
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(finalizer.count == 1,
                       "leak normal closure finalizer mismatch");
  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_leak_retain_release_case(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  i32_func closure = NULL;
  i32_func release_closure = NULL;
  i32_func add_one = NULL;
  i32_func returned_function = NULL;
  function_factory_func factory = NULL;
  tra_ffic_completion completion = NULL;
  finalize_counter finalizer;
  release_during_call_state release_state;
  typed_capture capture;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }

  memset(&finalizer, 0, sizeof(finalizer));
  if (!expect_true(tra_ffic_side_create_closure(
                       &context.side_b, &k_sig_echo_i32,
                       closure_echo_i32_function, &finalizer,
                       finalize_counter_callback, &closure, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  passed = expect_true(tra_ffic_function_retain(closure, &error),
                       error.message) &&
           passed;
  passed = expect_true(tra_ffic_function_retain(closure, &error),
                       error.message) &&
           passed;
  passed = expect_true(tra_ffic_function_release(closure, &error),
                       error.message) &&
           passed;
  passed = test_context_drain(&context) && passed;
  passed = passed && expect_true(finalizer.count == 0,
                                 "leak retained closure finalized early");
  passed = expect_true(tra_ffic_function_release(closure, &error),
                       error.message) &&
           passed;
  passed = test_context_drain(&context) && passed;
  passed = passed && expect_true(finalizer.count == 0,
                                 "leak retained closure finalized early");

  memset(&capture, 0, sizeof(capture));
  if (!expect_true(tra_ffic_side_create_completion_function(
                       &context.side_a, &k_type_i32, capture_i32_callback,
                       &completion, &capture, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  closure(completion, 8);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 8,
                       "leak retained closure result mismatch");
  passed = expect_true(tra_ffic_function_release(completion, &error),
                       error.message) &&
           passed;
  completion = NULL;
  passed = expect_true(tra_ffic_function_release(closure, &error),
                       error.message) &&
           passed;
  closure = NULL;
  passed = test_context_drain(&context) && passed;
  passed = passed && expect_true(finalizer.count == 1,
                                 "leak retained closure finalizer mismatch");

  memset(&finalizer, 0, sizeof(finalizer));
  memset(&capture, 0, sizeof(capture));
  memset(&release_state, 0, sizeof(release_state));
  release_state.finalizer = &finalizer;
  if (!expect_true(tra_ffic_side_create_closure(
                       &context.side_b, &k_sig_echo_i32,
                       release_during_call_function, &release_state,
                       finalize_release_during_call_callback, &release_closure,
                       &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  release_state.self = release_closure;
  if (!expect_true(tra_ffic_side_create_completion_function(
                       &context.side_a, &k_type_i32, capture_i32_callback,
                       &completion, &capture, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  release_closure(completion, 20);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 21,
                       "leak release-during-call result mismatch");
  passed = passed && expect_true(finalizer.count == 1,
                                 "leak release-during-call finalizer mismatch");
  if (finalizer.count == 1) {
    release_closure = NULL;
  }
  passed = expect_true(tra_ffic_function_release(completion, &error),
                       error.message) &&
           passed;
  completion = NULL;

  if (!expect_true(tra_ffic_side_create_pure_function(
                       &context.side_a, &k_sig_echo_i32, add_one_function,
                       &add_one, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  if (!expect_true(tra_ffic_side_create_closure(
                       &context.side_b, &k_sig_return_function,
                       return_function, &add_one, NULL, &factory, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  memset(&capture, 0, sizeof(capture));
  capture.retain_function = 1;
  if (!expect_true(tra_ffic_side_create_completion_function(
                       &context.side_a, &k_type_i32_function,
                       capture_function_callback, &completion, &capture,
                       &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  factory(completion);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.function_value == add_one,
                       "leak retained function result mismatch");
  if (capture.count == 1 && !capture.has_error &&
      capture.function_value != NULL) {
    returned_function = capture.function_value;
  }
  passed = expect_true(tra_ffic_function_release(completion, &error),
                       error.message) &&
           passed;
  completion = NULL;

  if (returned_function != NULL) {
    passed = expect_true(tra_ffic_function_release(returned_function, &error),
                         error.message) &&
             passed;
    returned_function = NULL;
  }

cleanup:
  if (completion != NULL) {
    passed = expect_true(tra_ffic_function_release(completion, &error),
                         error.message) &&
             passed;
  }
  if (returned_function != NULL) {
    passed = expect_true(tra_ffic_function_release(returned_function, &error),
                         error.message) &&
             passed;
  }
  if (factory != NULL) {
    passed = expect_true(tra_ffic_function_release(factory, &error),
                         error.message) &&
             passed;
  }
  if (add_one != NULL) {
    passed = expect_true(tra_ffic_function_release(add_one, &error),
                         error.message) &&
             passed;
  }
  if (release_closure != NULL) {
    passed = expect_true(tra_ffic_function_release(release_closure, &error),
                         error.message) &&
             passed;
  }
  if (closure != NULL) {
    passed = expect_true(tra_ffic_function_release(closure, &error),
                         error.message) &&
             passed;
  }
  passed = test_context_drain(&context) && passed;
  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_leak_nested_function_pointer_case(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  i32_func add_one = NULL;
  function_arg_func inner = NULL;
  function_arg_arg_func outer = NULL;
  tra_ffic_completion completion = NULL;
  nested_callback_state inner_state;
  nested_function_arg_state outer_state;
  typed_capture capture;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }

  if (!expect_true(tra_ffic_side_create_pure_function(
                       &context.side_a, &k_sig_echo_i32, add_one_function,
                       &add_one, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  inner_state.caller_side = &context.side_b;
  inner_state.queue = &context.queue;
  inner_state.drain_mode = drain_mode;
  if (!expect_true(tra_ffic_side_create_closure(
                       &context.side_b, &k_sig_function_arg,
                       call_nested_function, &inner_state, NULL, &inner,
                       &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  outer_state.caller_side = &context.side_b;
  outer_state.queue = &context.queue;
  outer_state.drain_mode = drain_mode;
  outer_state.inner_function = add_one;
  if (!expect_true(tra_ffic_side_create_closure(
                       &context.side_b, &k_sig_function_arg_arg,
                       call_nested_function_arg_function, &outer_state, NULL,
                       &outer, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  memset(&capture, 0, sizeof(capture));
  if (!expect_true(tra_ffic_side_create_completion_function(
                       &context.side_a, &k_type_i32, capture_i32_callback,
                       &completion, &capture, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  outer(completion, inner);
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.int32_value == 42,
                       "leak nested function pointer result mismatch");
  passed = expect_true(tra_ffic_function_release(completion, &error),
                       error.message) &&
           passed;
  completion = NULL;

cleanup:
  if (completion != NULL) {
    passed = expect_true(tra_ffic_function_release(completion, &error),
                         error.message) &&
             passed;
  }
  if (outer != NULL) {
    passed = expect_true(tra_ffic_function_release(outer, &error),
                         error.message) &&
             passed;
  }
  if (inner != NULL) {
    passed = expect_true(tra_ffic_function_release(inner, &error),
                         error.message) &&
             passed;
  }
  if (add_one != NULL) {
    passed = expect_true(tra_ffic_function_release(add_one, &error),
                         error.message) &&
             passed;
  }
  passed = test_context_drain(&context) && passed;
  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_leak_completion_case(test_drain_mode drain_mode) {
  test_context context;
  tra_ffic_error error;
  void_func string_function = NULL;
  void_func fail_function = NULL;
  void_func double_function = NULL;
  i32_func add_one = NULL;
  i32_func returned_function = NULL;
  function_factory_func factory = NULL;
  tra_ffic_completion completion = NULL;
  typed_capture capture;
  int passed = 1;
  if (!test_context_init(&context, drain_mode)) {
    return 0;
  }

  if (!expect_true(tra_ffic_side_create_pure_function(
                       &context.side_b, &k_sig_return_string,
                       return_stack_string_function, &string_function, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  memset(&capture, 0, sizeof(capture));
  if (!expect_true(tra_ffic_side_create_completion_function(
                       &context.side_a, &k_type_string,
                       capture_string_callback, &completion, &capture,
                       &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  string_function(completion);
  passed = expect_true(tra_ffic_function_release(completion, &error),
                       error.message) &&
           passed;
  completion = NULL;
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           strcmp(capture.string_value, "copied-string") == 0,
                       "leak completion string result mismatch");
  passed = expect_true(tra_ffic_function_release(string_function, &error),
                       error.message) &&
           passed;
  string_function = NULL;

  if (!expect_true(tra_ffic_side_create_pure_function(
                       &context.side_b, &k_sig_return_i32, fail_i32_function,
                       &fail_function, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  memset(&capture, 0, sizeof(capture));
  if (!expect_true(tra_ffic_side_create_completion_function(
                       &context.side_a, &k_type_i32, capture_i32_callback,
                       &completion, &capture, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  fail_function(completion);
  passed = expect_true(tra_ffic_function_release(completion, &error),
                       error.message) &&
           passed;
  completion = NULL;
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && capture.has_error &&
                           strcmp(capture.error_message,
                                  "expected failure") == 0,
                       "leak completion error mismatch");
  passed = expect_true(tra_ffic_function_release(fail_function, &error),
                       error.message) &&
           passed;
  fail_function = NULL;

  if (!expect_true(tra_ffic_side_create_pure_function(
                       &context.side_b, &k_sig_return_string,
                       double_completion_function, &double_function, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  memset(&capture, 0, sizeof(capture));
  if (!expect_true(tra_ffic_side_create_completion_function(
                       &context.side_a, &k_type_string,
                       capture_string_callback, &completion, &capture,
                       &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  double_function(completion);
  passed = expect_true(tra_ffic_function_release(completion, &error),
                       error.message) &&
           passed;
  completion = NULL;
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           strcmp(capture.string_value, "first") == 0,
                       "leak double completion result mismatch");
  passed = expect_true(tra_ffic_function_release(double_function, &error),
                       error.message) &&
           passed;
  double_function = NULL;

  if (!expect_true(tra_ffic_side_create_pure_function(
                       &context.side_a, &k_sig_echo_i32, add_one_function,
                       &add_one, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  if (!expect_true(tra_ffic_side_create_closure(
                       &context.side_b, &k_sig_return_function,
                       return_function, &add_one, NULL, &factory, &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  memset(&capture, 0, sizeof(capture));
  capture.retain_function = 1;
  if (!expect_true(tra_ffic_side_create_completion_function(
                       &context.side_a, &k_type_i32_function,
                       capture_function_callback, &completion, &capture,
                       &error),
                   error.message)) {
    passed = 0;
    goto cleanup;
  }
  factory(completion);
  passed = expect_true(tra_ffic_function_release(completion, &error),
                       error.message) &&
           passed;
  completion = NULL;
  passed = test_context_drain(&context) && passed;
  passed = passed &&
           expect_true(capture.count == 1 && !capture.has_error &&
                           capture.function_value == add_one,
                       "leak completion function result mismatch");
  if (capture.count == 1 && !capture.has_error &&
      capture.function_value != NULL) {
    returned_function = capture.function_value;
  }
  if (returned_function != NULL) {
    passed = expect_true(tra_ffic_function_release(returned_function, &error),
                         error.message) &&
             passed;
    returned_function = NULL;
  }

cleanup:
  if (completion != NULL) {
    passed = expect_true(tra_ffic_function_release(completion, &error),
                         error.message) &&
             passed;
  }
  if (returned_function != NULL) {
    passed = expect_true(tra_ffic_function_release(returned_function, &error),
                         error.message) &&
             passed;
  }
  if (factory != NULL) {
    passed = expect_true(tra_ffic_function_release(factory, &error),
                         error.message) &&
             passed;
  }
  if (add_one != NULL) {
    passed = expect_true(tra_ffic_function_release(add_one, &error),
                         error.message) &&
             passed;
  }
  if (double_function != NULL) {
    passed = expect_true(tra_ffic_function_release(double_function, &error),
                         error.message) &&
             passed;
  }
  if (fail_function != NULL) {
    passed = expect_true(tra_ffic_function_release(fail_function, &error),
                         error.message) &&
             passed;
  }
  if (string_function != NULL) {
    passed = expect_true(tra_ffic_function_release(string_function, &error),
                         error.message) &&
             passed;
  }
  passed = test_context_drain(&context) && passed;
  passed = test_context_destroy(&context) && passed;
  return passed;
}

static int run_leak_regression_test(test_drain_mode drain_mode) {
  int passed = 1;
  passed = run_leak_case("normal operation",
                         run_leak_normal_operation_case, drain_mode) &&
           passed;
  passed = run_leak_case("retain release", run_leak_retain_release_case,
                         drain_mode) &&
           passed;
  passed = run_leak_case("nested function pointer",
                         run_leak_nested_function_pointer_case,
                         drain_mode) &&
           passed;
  passed = run_leak_case("three-level function signatures",
                         run_three_level_function_signature_test,
                         drain_mode) &&
           passed;
  passed = run_leak_case("completion", run_leak_completion_case,
                         drain_mode) &&
           passed;
  return passed;
}

static int run_regression_test(test_drain_mode drain_mode) {
  int passed = 1;
  passed = passed && run_task_queue_notification_test(drain_mode);
  passed = passed && run_readme_minimum_test(drain_mode);
  passed = passed && run_primitive_test(drain_mode);
  passed = passed && run_basic_struct_test(drain_mode);
  passed = passed && run_closure_scalar_test(drain_mode);
  passed = passed && run_structured_scalar_call_test(drain_mode);
  passed = passed && run_success_scalar_call_test(drain_mode);
  passed = passed && run_retval_abi_test(drain_mode);
  passed = passed && run_buffer_view_test(drain_mode);
  passed = passed && run_completion_behavior_test(drain_mode);
  passed = passed && run_async_completion_test(drain_mode);
  passed = passed && run_pointer_list_argument_passing_test(drain_mode);
  passed = passed && run_function_marshalling_test(drain_mode);
  passed = passed && run_function_identity_test(drain_mode);
  passed = passed && run_three_level_function_signature_test(drain_mode);
  passed = passed && run_function_argument_error_test(drain_mode);
  passed = passed && run_lifetime_test(drain_mode);
  passed = passed && run_leak_regression_test(drain_mode);
  return passed;
}

int main(void) {
  const tra_ffic_closure_tracker_snapshot before =
      tra_ffic_get_closure_tracker_snapshot();
  int passed = 1;
  passed = passed && run_regression_test(TEST_DRAIN_INLINE);
  passed = passed && run_regression_test(TEST_DRAIN_THREAD);
  if (passed) {
    const tra_ffic_closure_tracker_snapshot after =
        tra_ffic_get_closure_tracker_snapshot();
    passed = passed &&
             expect_true(after.alloc_count > before.alloc_count,
                         "closure tracker did not observe allocations");
    passed = passed &&
             expect_true(after.alloc_count - before.alloc_count ==
                             after.free_count - before.free_count,
                         "closure tracker alloc/free mismatch");
    passed = passed &&
             expect_true(after.live_count == before.live_count,
                         "closure tracker live count did not return");
    passed = passed &&
             expect_true(after.high_water > before.high_water,
                         "closure tracker high-water did not increase");
  }
  return passed ? 0 : 1;
}
