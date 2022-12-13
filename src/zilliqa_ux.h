#ifndef ZIL_NANOS_UX_H
#define ZIL_NANOS_UX_H

#include "schnorr.h"
#include "zilliqa.h"
#include "qatozil.h"
#include "txn.pb.h"
#include "ux.h"
#ifdef HAVE_NBGL
#include "nbgl_use_case.h"
#endif

#define TXN_BUF_SIZE 256
#define TXN_DISP_CODE_MAX_LEN 500 // Probably quite generous on Nano screens...
#define TXN_DISP_DATA_MAX_LEN 500 // Probably quite generous on Nano screens...

typedef struct {
	uint32_t keyIndex;
	bool genAddr;
	uint8_t displayIndex;
	// NUL-terminated strings for display
	char typeStr[40]; // variable-length
	char keyStr[40]; // variable-length
	char fullStr[77]; // variable length
	// partialStr contains 12 characters of a longer string. This allows text
	// to be scrolled.
	char partialStr[13];
} getPublicKeyContext_t;

typedef struct {
	uint32_t keyIndex;
	uint8_t hash[32];
	char hexHash[65]; // 2*sizeof(hash) + 1 for '\0'
	uint8_t displayIndex;
	// NUL-terminated strings for display
	char indexStr[40]; // variable-length
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

	char toAddrStr[BECH32_ADDRSTR_LEN + 1];
	char amountStr[ZIL_UINT128_BUF_LEN];
	char gaspriceStr[ZIL_UINT128_BUF_LEN];
	char codeStr[TXN_DISP_CODE_MAX_LEN];
	char dataStr[TXN_DISP_DATA_MAX_LEN];
	ProtoTransactionCoreInfo txn;

	uint32_t displayIndex;
	char indexStr[40]; // variable-length
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