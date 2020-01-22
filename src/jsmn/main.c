#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#define BECH32_ADDRSTR_LEN (3 + 1 + 32 + 6)
#define PUB_ADDR_BYTES_LEN 20

#include "../txn_msg.h"

int (*bech32_addr_encode_ptr) (
    char *output,
    const char *hrp,
    const uint8_t *prog,
    size_t prog_len
) = NULL;

void (*hex2bin_ptr)(const uint8_t *hexstr, unsigned numhexchars, uint8_t *bin) = NULL;

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
