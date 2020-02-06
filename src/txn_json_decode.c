#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "txn_json_decode.h"

int strncmp( const char * s1, const char * s2, size_t n );
size_t strlen(const char *str);
char *strcat(char *dest, const char *src);
char *strncat(char *dest, const char *src, size_t n);

static inline bool json_string_eq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return true;
  }
  return false;
}

static inline jsmntok_t *access (Tokens *tokens, int idx)
{
  if (idx >= tokens->total_tokens) {
    FAIL("Internal error: Invalid access: out-of-bounds token\n");
  }

  return &tokens->tokens[idx];  
}

// The next token at this depth, skipping all sub-tokens.
static inline int next_token (Tokens *tokens, int skip_idx)
{
  switch (access(tokens, skip_idx)->type) {
  case JSMN_UNDEFINED:
    FAIL("Internal error: JSMN_UNDEFINED encountered\n");
    break;
  case JSMN_STRING:
  case JSMN_PRIMITIVE:
    return skip_idx + 1;
  case JSMN_OBJECT:
  {
    int i = skip_idx+1;
    for (int child = 0; child < access(tokens, skip_idx)->size; child++) {
      /* i is now positiioned at the "key" part of the key:val. Move it to the "val" part. */
      i += 1;
      /* i is now positioned to the "val" part of the key:val, skip the "val" part. */
      i = next_token(tokens, i);
    }
    return i;
  }
  case JSMN_ARRAY:
  {
    int i = skip_idx + 1;
    for (int child = 0; child < access(tokens, skip_idx)->size; child++) {
      i = next_token(tokens, i);
    }
    return i;
  }
  }

  FAIL("Internal error: skip_token: unreachable");
  return 0;
}

// Return the index of the "value" token of "key".
// Returns -1 if unsuccessful.
static inline int find_key_in_object(const char *json, Tokens *tokens, int object_idx, const char* key)
{
  if (access(tokens, object_idx)->type != JSMN_OBJECT)
    return -1;

  // Look inside the object, not moving past its children.
  int key_tok = object_idx + 1;
  for (int child = 0; child < access (tokens, object_idx)->size; child++)
  {
    switch (access(tokens, key_tok)->type) {
    case JSMN_STRING:
      if (json_string_eq(json, access(tokens, key_tok), key))
        return key_tok+1;
      /* Not the key we want, skip the value part to go to the next key. */
      key_tok = next_token(tokens, key_tok + 1);
      break;
    default:
      FAIL("Internal Error: Object JSON has non-string key\n");
      return -1;
    }
  }

  PRINTF("Key %s not found\n", key);
  return -1;
}

// Print a scilla value JSON object { "vname" : ..., "type" : ..., "value" : ... }
// Returns number of characters outputted to the output buffer. Negative on failure.
static inline int print_scilla_value_object(const char *json, Tokens *tokens, int object_idx, char *output, int output_size)
{
  if (access(tokens, object_idx)->type != JSMN_OBJECT) {
    PRINTF("Scilla value found to be not an Object");
    return -1;
  }

  int vname_idx = find_key_in_object(json, tokens, object_idx, "vname");
  int value_idx = find_key_in_object(json, tokens, object_idx, "value");
  int type_idx = find_key_in_object(json, tokens, object_idx, "type");
  if (vname_idx < 0 || value_idx < 0 || type_idx < 0 ||
      access(tokens, vname_idx)->type != JSMN_STRING ||
      access(tokens, value_idx)->type != JSMN_STRING ||
      access(tokens, type_idx)->type != JSMN_STRING)
  {
    PRINTF("Malformed Scilla value JSON Object\n");
    return -1;
  }

  int num_chars_vname = access(tokens, vname_idx)->end - access(tokens, vname_idx)->start;
  const char *vname_start = json + access(tokens, vname_idx)->start;
  int num_chars_value = access(tokens, value_idx)->end - access(tokens, value_idx)->start;
  const char *value_start = json + access(tokens, value_idx)->start;

#ifdef JSON_BECH32_ADDR

  void hex2bin(const uint8_t *hexstr, unsigned numhexchars, uint8_t *bin);
  int bech32_addr_encode(
    char *output,
    const char *hrp,
    const uint8_t *prog,
    size_t prog_len
  );
  char value_buf[BECH32_ENCODE_BUF_LEN];
  unsigned char bystr20[20];

  CHECK_CANARY;

  // Is this a ByStr20 type Scilla value.
  bool is_addr = json_string_eq(json, access(tokens, type_idx), "ByStr20");

  // If address value, attempt bech32 conversion.
  if (is_addr && num_chars_value == (strlen ("0x") + 40))
  {
    PRINTF("Converting address in txn data JSON to bech32\n");
    hex2bin((uint8_t *) json + access(tokens, value_idx)->start, 40, bystr20);
    CHECK_CANARY;
    if (!bech32_addr_encode(value_buf, "zil", bystr20, PUB_ADDR_BYTES_LEN)) {
      CHECK_CANARY;
      FAIL ("bech32 encoding of address failed\n");
    }
    num_chars_value = BECH32_ADDRSTR_LEN;
    value_start = value_buf;
  }

	CHECK_CANARY;

#endif

  const char *sep = ":";
  // Add these up and one more for '\0' added by strncat.
  int num_chars_required = num_chars_vname + strlen (sep) + num_chars_value + 1;

  if (output_size < num_chars_required)
    return -1;

  output[0] = '\0';

  strncat(output, vname_start, num_chars_vname);
  strcat(output, sep);
  strncat(output, value_start, num_chars_value);

  /* We don't need the last null character put in by strncat, so don't account for it. */
  return num_chars_required - 1;
}

// Parse and print interesting data in JSON.
// Returns number of chars written to output, negative on failure.
int process_json(Tokens *tokens, const char jsonBuf[], int jsonBufLen, char *output, int output_size)
{
  CHECK_CANARY;

  jsmn_parser p;

  jsmn_init(&p);
  tokens->total_tokens = jsmn_parse(&p, jsonBuf, jsonBufLen, tokens->tokens, MAX_NUM_TOKENS);

  CHECK_CANARY;

#if 0
  for (int i = 0; i < tokens.total_tokens; i++) {
    PRINTF("%d: ", i);
    switch (access(&tokens, i)->type) {
    case JSMN_UNDEFINED:
      PRINTF("UNDEFINED: "); break;
    case JSMN_OBJECT:
      PRINTF("OBJECT: "); break;
    case JSMN_ARRAY:
      PRINTF("ARRAY: "); break;
    case JSMN_STRING:
      PRINTF("STRING: "); break;
    case JSMN_PRIMITIVE:
      PRINTF("PRIMITIVE: "); break;
    }
    PRINTF("start=%d, end=%d, size=%d, next_token=%d\n", access(&tokens, i)->start,
           access(&tokens, i)->end, access(&tokens, i)->size, next_token(&tokens, i));
  }
#endif

  CHECK_CANARY;

  if (tokens->total_tokens < 1) {
    PRINTF("Empty JSON\n");
    return -1;
  }

  // Find the key "params" in the outer Object.
  int params_idx = find_key_in_object(jsonBuf, tokens, 0, "params");
  if (params_idx < 0) {
    PRINTF ("Could not find \"params\" in JSON\n");
    return -1;
  }

  CHECK_CANARY;

  // If "params" value is not an array, it's malformed.
  if (access(tokens, params_idx)->type != JSMN_ARRAY) {
    PRINTF ("Expected \"params\" to be a JSON array\n");
    return -1;
  }

  CHECK_CANARY;

  // Print all params.
  int param_idx = params_idx + 1;
  int num_chars_total = 0;
  for (int child = 0; child < access(tokens, params_idx)->size; child++) {
    if (access(tokens, param_idx)->type != JSMN_OBJECT) {
      PRINTF("Expected Object item in \"params\" array.\n");
      return -1;
    }

    int num_chars = print_scilla_value_object(jsonBuf, tokens, param_idx, output, output_size);
    if (num_chars < 0)
      return num_chars;

    num_chars_total += num_chars;
    output += num_chars;
    output_size -= num_chars;

    // Add a space to separate.
    if (output_size > 0) {
      num_chars_total++;
      output[0] = ' ';
      output++;
      output_size--;
    }
    
    param_idx = next_token(tokens, param_idx);
  }

  CHECK_CANARY;

  return num_chars_total;
}
