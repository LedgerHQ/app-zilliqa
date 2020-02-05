#ifndef TXN_JSON_DECODE_H
#define TXN_JSON_DECODE_H

#include "zilliqa.h"

// We want the addresses in JSON data displayed in BECH32
// #define JSON_BECH32_ADDR

int process_json(const char buf[], int bufLen, char *output, int output_size);

#endif // TXN_JSON_DECODE_H
