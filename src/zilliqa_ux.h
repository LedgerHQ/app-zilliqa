#ifndef ZIL_NANOS_UX_H
#define ZIL_NANOS_UX_H

#include "schnorr.h"
#include "zilliqa.h"
#include "txn.pb.h"
#include "ux.h"

#define TXN_BUF_SIZE 256
#define TXN_DISP_MSG_MAX_LEN 512

typedef struct {
	uint32_t keyIndex;
	bool genAddr;
	uint8_t displayIndex;
	// NUL-terminated strings for display
	uint8_t typeStr[40]; // variable-length
	uint8_t keyStr[40]; // variable-length
	uint8_t fullStr[77]; // variable length
	// partialStr contains 12 characters of a longer string. This allows text
	// to be scrolled.
	uint8_t partialStr[13];
} getPublicKeyContext_t;

typedef struct {
	uint32_t keyIndex;
	uint8_t hash[32];
	uint8_t hexHash[65]; // 2*sizeof(hash) + 1 for '\0'
	uint8_t displayIndex;
	// NUL-terminated strings for display
	uint8_t indexStr[40]; // variable-length
	uint8_t partialHashStr[13];
} signHashContext_t;

typedef struct {
	uint8_t buf[TXN_BUF_SIZE];
	uint32_t nextIdx, len; // next read into buf and len of buf.
	int hostBytesLeft;     // How many more bytes to be streamed from host.
} StreamData;

typedef struct {
	uint32_t keyIndex;
	zil_ecschnorr_t ecs;
	uint8_t signature[SCHNORR_SIG_LEN_RS];
  StreamData sd;

	uint8_t msg[TXN_DISP_MSG_MAX_LEN + 1]; // last byte for '\0'
	unsigned int msgLen;
	ProtoTransactionCoreInfo txn;

	uint32_t displayIndex;
	uint8_t indexStr[40]; // variable-length
	uint8_t partialMsg[13];
} signTxnContext_t;

// To save memory, we store all the context types in a single global union,
// taking advantage of the fact that only one command is executed at a time.
typedef union {
	getPublicKeyContext_t getPublicKeyContext;
	signHashContext_t signHashContext;
	signTxnContext_t signTxnContext;
} commandContext;
extern commandContext global;

// ui_idle displays the main menu screen. Command handlers should call ui_idle
// when they finish.
void ui_idle(void);

// io_exchange_with_code is a helper function for sending APDUs, primarily
// from button handlers. It appends code to G_io_apdu_buffer and calls
// io_exchange with the IO_RETURN_AFTER_TX flag. tx is the current offset
// within G_io_apdu_buffer (before the code is appended).
void io_exchange_with_code(uint16_t code, uint16_t tx);

#endif