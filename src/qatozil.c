#include <stdbool.h>
#include <stdint.h>
#ifndef UNIT_TESTS
#include <string.h>
#else
#include <bsd/string.h>
#endif

#include "qatozil.h"
#ifndef UNIT_TESTS
#include "zilliqa.h"
#else
#include <assert.h>
#define CHECK_CANARY
#endif

int isdigit(int);

/* Filter out leading zero's and non-digit characters in a null terminated string. */
static void cleanse_input(char *buf)
{
  int len = strnlen(buf, ZIL_UINT128_BUF_LEN);
  assert (len < ZIL_UINT128_BUF_LEN);
  int nextpos = 0;
  bool seen_nonzero = false;

  for (int i = 0; i < len; i++) {
    char c = buf[i];
    if (c == '0' && !seen_nonzero) {
      continue;
    }
    if (isdigit(c)) {
      seen_nonzero = true;
      buf[nextpos++] = c;
    }
  }
  assert (nextpos <= len);

  if (nextpos == 0)
    buf[nextpos++] = '0';

  buf[nextpos] = '\0';
}

/* Removing trailing 0s and ".". */
static void remove_trailing_zeroes(char *buf)
{
  int len = strnlen(buf, ZIL_UINT128_BUF_LEN);
  assert(len < ZIL_UINT128_BUF_LEN);

  for (int i = len-1; i >= 0; i--) {
    if (buf[i] == '0')
      buf[i] = '\0';
    else if (buf[i] == '.') {
      buf[i] = '\0';
      break;
    } else {
      break;
    }
  }
}

#define QA_ZIL_SHIFT 12

/* Given a null terminated sequence of digits (value < UINT128_MAX),
 * divide it by "shift" and pretty print the result. */
static void ToZil(char *input, char *output, int output_len, int shift)
{
  int len = strnlen(input, ZIL_UINT128_BUF_LEN);
  assert(len > 0 && len < ZIL_UINT128_BUF_LEN);
  assert(shift == QA_ZIL_SHIFT);

  if (len <= shift) {
    strlcpy(output, "0.", output_len);
    /* Insert (shift - len) 0s. */
    for (int i = 0; i < (shift - len); i++) {
      /* A bit inefficient, but it's ok, at most shift iterations. */
      strlcat(output, "0", output_len);
    }
    strlcat(output, input, output_len);
    remove_trailing_zeroes(output);
    return;
  }

  /* len >= shift+1. Copy the first len-shift characters. */
  strncpy(output, input, len - shift);
  /* append a decimal point. */
  strlcpy(output + len - shift, ".", output_len - len + shift);
  /* copy the remaining characters in input. */
  strlcat(output, input + len - shift, output_len);
  /* Remove trailing zeroes (after the decimal point). */
  remove_trailing_zeroes(output);
}

void qa_to_zil(const char* qa, char* zil_buf, int zil_buf_len)
{
  int qa_len = strnlen(qa, ZIL_UINT128_BUF_LEN);
  assert(zil_buf_len >= ZIL_UINT128_BUF_LEN && qa_len < ZIL_UINT128_BUF_LEN);

  char qa_buf[ZIL_UINT128_BUF_LEN];

  CHECK_CANARY;
  strlcpy(qa_buf, qa, sizeof(qa_buf));
  /* Cleanse the input. */
  cleanse_input(qa_buf);
  /* Convert Qa to Zil. */
  ToZil(qa_buf, zil_buf, zil_buf_len, QA_ZIL_SHIFT);
  CHECK_CANARY;
}
