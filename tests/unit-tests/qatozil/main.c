#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>

#include "qatozil.h"

int main(int argc, char *argv[])
{
  const char *buf;
  char zilbuf[ZIL_UINT128_BUF_LEN];

  if (argc == 2) {
    buf = argv[1];
  } else {
    fprintf(stderr, "Usage:./qatozil Qa (length of Qa <= 39 digits)\n");
    exit(1);
  }

  qa_to_zil(buf, zilbuf, sizeof(zilbuf));
  /* Print output. */
  printf("%s\n", zilbuf);

  return 0;
}
