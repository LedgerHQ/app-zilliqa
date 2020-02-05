#ifndef TXN_JSON_DECODE_H
#define TXN_JSON_DECODE_H

// Define few macros that is only define din Ledger and not available
// to us for standalone testing.

#define MAX(X, Y) (X) > (Y) ? (X) : (Y)
#define MIN(X, Y) (X) < (Y) ? (X) : (Y)

#include "stdio.h"
#define PRINTF(...) fprintf(stderr, __VA_ARGS__)

#include <stdlib.h>
#define FAIL(...) PRINTF(__VA_ARGS__); abort()

#define CHECK_CANARY

int process_json(const char buf[], int bufLen, char *output, int output_size);

#endif // TXN_JSON_DECODE_H
