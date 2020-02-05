#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "../txn_json_decode.h"

#define MAX_BUF_LEN 512

int main(int argc, char* argv[])
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s test.json\n", argv[0]);
    return 1;
  }

  FILE *f = fopen(argv[1], "r");
  if (!f) {
    fprintf(stderr, "Error opening file %s\n", argv[1]);
    return 1;
  }
  char buf[MAX_BUF_LEN];

  int bufLen;
  if ((bufLen = fread(buf, 1, MAX_BUF_LEN, f)) == 0 || !feof(f)) {
    fprintf(stderr, "Error loading file %s\n", argv[1]);
    return 1;
  }

  char output[MAX_BUF_LEN];
  int output_len;
  if ((output_len = process_json(buf, bufLen, output, MAX_BUF_LEN)) < 0) {
    fprintf(stderr, "Error processing JSON %s\n", argv[1]);
    return 1;
  }

  for (int i = 0; i < output_len; i++)
    putchar(output[i]);

  putchar('\n');
  
  return 0;
}
