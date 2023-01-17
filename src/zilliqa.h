#ifndef ZIL_NANOS_ZILLIQA_H
#define ZIL_NANOS_ZILLIQA_H

#include "schnorr.h"

// These are the offsets of various parts of a request APDU packet. INS
// identifies the requested command (see above), and P1 and P2 are parameters
// to the command.
#define CLA          0xE0
#define OFFSET_CLA   0x00
#define OFFSET_INS   0x01
#define OFFSET_P1    0x02
#define OFFSET_P2    0x03
#define OFFSET_LC    0x04
#define OFFSET_CDATA 0x05

// Use Zilliqa's DER decode function for signing?
// (this shouldn't have any functional impact).
#define DER_DECODE_ZILLIQA 0

// MACROS
#define PLOC() PRINTF("\n%s - %s:%d \n", __FILE__, __func__, __LINE__);
#define assert(x) \
    if (x) {} else { PLOC(); PRINTF("Assertion failed\n"); THROW (EXCEPTION); }
#define FAIL(x) \
    { \
        PLOC();\
        PRINTF("Zilliqa ledger app failed: %s\n", x);\
        THROW(EXCEPTION); \
    }

#ifdef HAVE_BOLOS_APP_STACK_CANARY
// This symbol is defined by the link script to be at the start of the stack
// area.
extern unsigned long _stack, _estack;
#define STACK_CANARY (*((volatile uint32_t*) &_stack))

void print_available_stack();

#define INIT_CANARY                                              \
  STACK_CANARY = 0xDEADBEEF;                                     \
  PLOC();                                                        \
  PRINTF("init_canary: initialized at STACK_START 0x%p and STACK_END 0x%p with STACK_SIZE=%d\n", \
  &_stack, &_estack, ((uintptr_t)&_estack) - ((uintptr_t)&_stack)); \
  print_available_stack();

#define CHECK_CANARY                             \
  if (STACK_CANARY != 0xDEADBEEF)                \
    FAIL("check_canary: EXCEPTION_OVERFLOW");    \
  PLOC(); print_available_stack();               \
  PRINTF("check_canary: successfull\n");

#else

#define INIT_CANARY
#define CHECK_CANARY

#endif // HAVE_BOLOS_APP_STACK_CANARY


// Constants
#define SHA256_HASH_LEN 32
#define PUB_ADDR_BYTES_LEN 20
#define PUBLIC_KEY_BYTES_LEN 33
// https://github.com/Zilliqa/Zilliqa/wiki/Address-Standard#specification
#define BECH32_ADDRSTR_LEN (3 + 1 + 32 + 6)
#define SCHNORR_SIG_LEN_RS 64
#define ZIL_AMOUNT_GASPRICE_BYTES 16
#define ZIL_MAX_TXN_SIZE 8388608 // 8MB
// bech32_addr_encode requires 73 + strlen("zil") sized buffer.
#define BECH32_ENCODE_BUF_LEN 73 + 3

// exception codes
#define SW_WRONG_DATA_LENGTH 0x6A87
#define SW_INS_NOT_SUPPORTED 0x6D00
#define SW_CLA_NOT_SUPPORTED 0x6E00
#define SW_DEVELOPER_ERR     0x6B00
#define SW_INVALID_PARAM     0x6B01
#define SW_IMPROPER_INIT     0x6B02
#define SW_USER_REJECTED     0x6985
#define SW_OK                0x9000

// macros for converting raw bytes to uint64_t
#define U8BE(buf, off) (((uint64_t)(U4BE(buf, off))     << 32) | ((uint64_t)(U4BE(buf, off + 4)) & 0xFFFFFFFF))
#define U8LE(buf, off) (((uint64_t)(U4LE(buf, off + 4)) << 32) | ((uint64_t)(U4LE(buf, off))     & 0xFFFFFFFF))

// FUNCTIONS

// Convert un-compressed zilliqa public key to a compressed form.
void compressPubKey(cx_ecfp_public_key_t *publicKey);

// pubkeyToZilAddress converts a Ledger pubkey to a Zilliqa wallet address.
void pubkeyToZilAddress(uint8_t *dst, cx_ecfp_public_key_t *publicKey);

// deriveZilPubKey derives an Ed25519 key pair from an index and the Ledger
// seed. Returns the public key (private key is not needed).
void deriveZilPubKey(uint32_t index, cx_ecfp_public_key_t *publicKey);

// Three functions to stream the signature process. See deriveAndSign to do in a single operation.
void deriveAndSignInit(zil_ecschnorr_t *T, uint32_t index);
void deriveAndSignContinue(zil_ecschnorr_t *T, const uint8_t *msg, unsigned int msg_len);
int deriveAndSignFinish(zil_ecschnorr_t *T, uint32_t index, unsigned char *dst, unsigned int dst_len);

// deriveAndSign derives an ECFP private key from an user specified index and the Ledger seed,
// and uses it to produce a SCHNORR_SIG_LEN_RS length signature of the provided message
// The key is cleared from memory after signing.
void deriveAndSign(uint8_t *dst, uint32_t dst_len, uint32_t index, const uint8_t *msg, unsigned int msg_len);

#endif