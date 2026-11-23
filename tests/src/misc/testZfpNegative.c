#include "zfp.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>

#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

/* utility functions ------------------------------------------------------- */

static zfp_stream*
open_stream(const zfp_field* field)
{
  size_t size;
  void* buffer;
  bitstream* bs;
  zfp_stream* zfp = zfp_stream_open(NULL);

  zfp_stream_set_reversible(zfp);
  size = zfp_stream_maximum_size(zfp, field);
  buffer = calloc(size, 1);
  bs = stream_open(buffer, size);
  zfp_stream_set_bit_stream(zfp, bs);

  return zfp;
}

static void
close_stream(zfp_stream* zfp)
{
  bitstream* bs = zfp_stream_bit_stream(zfp);
  void* buffer = stream_data(bs);

  zfp_stream_close(zfp);
  stream_close(bs);
  free(buffer);
}

/* tests ------------------------------------------------------------------- */

static void
given_undefined_type_when_compress_expect_failure(void **state)
{
  float array[] = { 1, 2, 3, 4 };
  zfp_field* field = zfp_field_1d(array, zfp_type_none, 4);
  zfp_stream* zfp = open_stream(field);

  /* expect failure (zero return value) when scalar type is undefined */
  assert_int_equal(zfp_compress(zfp, field), 0);

  close_stream(zfp);
  zfp_field_free(field);
}

static void
given_undefined_type_when_decompress_expect_failure(void **state)
{
  float array[4];
  zfp_field* field = zfp_field_1d(array, zfp_type_none, 4);
  zfp_stream* zfp = open_stream(field);

  /* expect failure (zero return value) when scalar type is undefined */
  assert_int_equal(zfp_decompress(zfp, field), 0);

  close_stream(zfp);
  zfp_field_free(field);
}

static void
given_empty_field_when_compress_expect_failure(void **state)
{
  zfp_field* field = zfp_field_alloc();
  zfp_stream* zfp = open_stream(field);

  /* expect failure (zero return value) when field has no dimensions */
  assert_int_equal(zfp_compress(zfp, field), 0);

  close_stream(zfp);
  zfp_field_free(field);
}

static void
given_empty_field_when_decompress_expect_failure(void **state)
{
  zfp_field* field = zfp_field_alloc();
  zfp_stream* zfp = open_stream(field);

  /* expect failure (zero return value) when field has no dimensions */
  assert_int_equal(zfp_decompress(zfp, field), 0);

  close_stream(zfp);
  zfp_field_free(field);
}

static void
given_null_field_data_pointer_when_compress_expect_failure(void **state)
{
  zfp_field* field = zfp_field_3d(NULL, zfp_type_float, 3, 4, 5);
  zfp_stream* zfp = open_stream(field);

  /* expect failure (zero return value) when field data pointer is null */
  assert_int_equal(zfp_compress(zfp, field), 0);

  close_stream(zfp);
  zfp_field_free(field);
}

static void
given_null_field_data_pointer_when_decompress_expect_failure(void **state)
{
  zfp_field* field = zfp_field_3d(NULL, zfp_type_float, 3, 4, 5);
  zfp_stream* zfp = open_stream(field);

  /* expect failure (zero return value) when field data pointer is null */
  assert_int_equal(zfp_decompress(zfp, field), 0);

  close_stream(zfp);
  zfp_field_free(field);
}

int main()
{
  /* negative unit tests that exercise invalid inputs */
  const struct CMUnitTest tests[] = {
    cmocka_unit_test(given_undefined_type_when_compress_expect_failure),
    cmocka_unit_test(given_undefined_type_when_decompress_expect_failure),
    cmocka_unit_test(given_empty_field_when_compress_expect_failure),
    cmocka_unit_test(given_empty_field_when_decompress_expect_failure),
    cmocka_unit_test(given_null_field_data_pointer_when_compress_expect_failure),
    cmocka_unit_test(given_null_field_data_pointer_when_decompress_expect_failure),
  };

  return cmocka_run_group_tests(tests, NULL, NULL);
}
