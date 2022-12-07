#ifndef ZIL_QATOZIL_H
#define ZIL_QATOZIL_H

// UINT128_MAX has 39 digits. Another 3 digits for "0." and '\0'.
// ("0." is prepended when converted Qa to Zil).
#define ZIL_UINT128_BUF_LEN 42

// Converts a null-terminated buffer containing Qa to Zil / Li
// assert (zil/li_buf_len >= ZIL_UINT128_BUF_LEN);
void qa_to_zil(const char* qa, char* zil_buf, int zil_buf_len);

#endif
