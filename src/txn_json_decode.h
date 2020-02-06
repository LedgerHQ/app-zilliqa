#ifndef TXN_JSON_DECODE_H
#define TXN_JSON_DECODE_H

#include "jsmn.h"
#include "zilliqa.h"

// We want the addresses in JSON data displayed in BECH32
#define JSON_BECH32_ADDR

#define MAX_NUM_TOKENS 24

typedef struct
{
  jsmntok_t tokens[MAX_NUM_TOKENS];
  int total_tokens;
} Tokens;

int process_json(Tokens *tokens, const char jsonBuf[], int jsonBufLen, char *output, int output_size);

#endif // TXN_JSON_DECODE_H
