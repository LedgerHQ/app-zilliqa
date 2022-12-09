#include <stdint.h>
#include <stdbool.h>

#include "os.h"
#include "os_io_seproxyhal.h"
#include "zilliqa.h"
#include "qatozil.h"
#include "zilliqa_ux.h"
#include "pb_decode.h"
#include "txn.pb.h"
#include "uint256.h"
#include "bech32_addr.h"

static signTxnContext_t * const ctx = &global.signTxnContext;

static void do_approve(const bagl_element_t *e)
{
		assert(IO_APDU_BUFFER_SIZE >= SCHNORR_SIG_LEN_RS);
		memcpy(G_io_apdu_buffer, ctx->signature, SCHNORR_SIG_LEN_RS);
		// Send the data in the APDU buffer, which is a 64 byte signature.
		io_exchange_with_code(SW_OK, SCHNORR_SIG_LEN_RS);
		// Return to the main screen.
		ui_idle();
}

static void do_reject(const bagl_element_t *e)
{
    io_exchange_with_code(SW_USER_REJECTED, 0);
    ui_idle();
}

UX_FLOW_DEF_NOCB(
    ux_signmsg_flow_1_step,
    pnn,
    {
      &C_icon_certificate,
      "Sign Txn",
      ctx->indexStr,
    });
UX_FLOW_DEF_NOCB(
    ux_signmsg_flow_2_step,
    bnnn_paging,
    {
      .title = "To",
      .text = ctx->toAddrStr,
    });
UX_FLOW_DEF_NOCB(
    ux_signmsg_flow_3_step,
    bnnn_paging,
    {
      .title = "Amount (ZIL)",
      .text = ctx->amountStr,
    });
UX_FLOW_DEF_NOCB(
    ux_signmsg_flow_4_step,
    bnnn_paging,
    {
      .title = "Gasprice (ZIL)",
      .text = ctx->gaspriceStr,
    });
UX_FLOW_DEF_NOCB(
    ux_signmsg_flow_5_step,
    bnnn_paging,
    {
      .title = "Contract code",
      .text = ctx->codeStr,
    });
UX_FLOW_DEF_NOCB(
    ux_signmsg_flow_6_step,
    bnnn_paging,
    {
      .title = "Contract data",
      .text = ctx->dataStr,
    });
UX_FLOW_DEF_VALID(
    ux_signmsg_flow_7_step,
    pn,
    do_approve(NULL),
    {
      &C_icon_validate_14,
      "Sign",
    });
UX_FLOW_DEF_VALID(
    ux_signmsg_flow_8_step,
    pn,
    do_reject(NULL),
    {
      &C_icon_crossmark,
      "Cancel",
    });

/* Simple flow without Smart Contract code nor data */
UX_FLOW(ux_signmsg_simple_flow,
  &ux_signmsg_flow_1_step,
  &ux_signmsg_flow_2_step,
  &ux_signmsg_flow_3_step,
  &ux_signmsg_flow_4_step,
  &ux_signmsg_flow_7_step,
  &ux_signmsg_flow_8_step);

/* Flow without Smart Contract code but with data */
UX_FLOW(ux_signmsg_data_flow,
  &ux_signmsg_flow_1_step,
  &ux_signmsg_flow_2_step,
  &ux_signmsg_flow_3_step,
  &ux_signmsg_flow_4_step,
  &ux_signmsg_flow_6_step,
  &ux_signmsg_flow_7_step,
  &ux_signmsg_flow_8_step);

/* Flow with Smart Contract code and data */
UX_FLOW(ux_signmsg_code_data_flow,
  &ux_signmsg_flow_1_step,
  &ux_signmsg_flow_2_step,
  &ux_signmsg_flow_3_step,
  &ux_signmsg_flow_4_step,
  &ux_signmsg_flow_5_step,
  &ux_signmsg_flow_6_step,
  &ux_signmsg_flow_7_step,
  &ux_signmsg_flow_8_step);

static bool istream_callback (pb_istream_t *stream, pb_byte_t *buf, size_t count)
{
	StreamData *sd = stream->state;
	int bufNext = 0;
	CHECK_CANARY;
	PRINTF("istream_callback: sd->nextIdx = %d\n", sd->nextIdx);
	PRINTF("istream_callback: sd->len = %d\n", sd->len);
	int sdbufRem = sd->len - sd->nextIdx;
	if (sdbufRem > 0) {
		// We have some data to spare.
		int copylen = MIN(sdbufRem, (int)count);
		memcpy(buf, sd->buf + sd->nextIdx, copylen);
		count -= copylen;
		bufNext += copylen;
		sd->nextIdx += copylen;
		PRINTF("Streamed %d bytes of data.\n", copylen);
	}

	if (count > 0) {
		// More data to be streamed, but we've run out. Stream from host.
		PRINTF("Still need to stream %d bytes of data.\n", count);
		assert(sd->len == sd->nextIdx);
		if (sd->hostBytesLeft) {
			G_io_apdu_buffer[0] = 0x90;
			G_io_apdu_buffer[1] = 0x00;
			unsigned rx = io_exchange(CHANNEL_APDU, 2);
			// Sanity-check the command length
			if (rx < OFFSET_CDATA) {
				FAIL("Bad command length");
			}
			// APDU length and LC field consistency
			if (rx - OFFSET_CDATA != G_io_apdu_buffer[OFFSET_LC]) {
				FAIL("Bad command length");
			}
			static const uint32_t hostBytesLeftOffset = OFFSET_CDATA + 0;
			static const uint32_t txnLenOffset = OFFSET_CDATA + 4;
			static const uint32_t dataOffset = OFFSET_CDATA + 8;

			// Sanity-check the command length
			if (rx < OFFSET_CDATA + sizeof(uint32_t) + sizeof(uint32_t)) {
				FAIL("Bad command length");
			}

			// These two cannot be made static as the function is recursive.
			uint32_t hostBytesLeft = U4LE(G_io_apdu_buffer, hostBytesLeftOffset);
			uint32_t txnLen = U4LE(G_io_apdu_buffer, txnLenOffset);
			PRINTF("istream_callback: io_exchanged %d bytes\n", rx);
			PRINTF("istream_callback: hostBytesLeft: %d\n", hostBytesLeft);
			PRINTF("istream_callback: txnLen: %d\n", txnLen);
			if (rx != dataOffset + txnLen) {
				FAIL("Bad command length");
			}
			if (txnLen > TXN_BUF_SIZE) {
				FAIL("Cannot handle large data sent from host");
			}
			assert(hostBytesLeft <= ZIL_MAX_TXN_SIZE - txnLen);

			// Update and move data to our state.
			sd->len = txnLen;
			memcpy(sd->buf, G_io_apdu_buffer + dataOffset, txnLen);
			sd->hostBytesLeft = hostBytesLeft;
			sd->nextIdx = 0;
			CHECK_CANARY;
			// Take care of updating our signature state.
			deriveAndSignContinue(&ctx->ecs, sd->buf, txnLen);

			PRINTF("Making recursive call to stream after io_exchange\n");
			return istream_callback(stream, buf+bufNext, count);
		} else {
			// We need more data but can't fetch again. This is an error.
			FAIL("Ran out of data to stream from host");
		}
	}

	return true;
}


static bool decode_and_store_in_ctx(pb_istream_t *stream, char* buffer, uint32_t buffer_len)
{
	size_t jsonLen = stream->bytes_left;
	PRINTF("decode_and_store_in_ctx: data length=%d\n", jsonLen);
	if (jsonLen + 1 /* one byte for \0 */ > buffer_len) {
		PRINTF("decode_txn_data: Cannot decode code, too large.\n");
		strlcpy(buffer, "Error: Too large", buffer_len);
		// We can't do anything but just consume the data.
		if (!pb_read(stream, NULL, jsonLen)) {
			FAIL("pb_read failed during txn data decode");
		}
		return true;
	}

	PRINTF("decode_txn_data: Displaying raw JSON of length %d\n", jsonLen);
	if (!pb_read(stream, (pb_byte_t*) buffer, jsonLen)) {
		FAIL("pb_read failed during txn data decode");
	}
	// Ensure the string is '\0' terminated
	buffer[jsonLen] = '\0';

	return true;
}

static bool decode_code_callback (pb_istream_t *stream, const pb_field_t *field, void **arg)
{
	if (field->tag != ProtoTransactionCoreInfo_code_tag) {
		FAIL("Unexpected data");
	}
	return decode_and_store_in_ctx(stream, ctx->codeStr, sizeof(ctx->codeStr));
}

static bool decode_data_callback (pb_istream_t *stream, const pb_field_t *field, void **arg)
{
	if (field->tag != ProtoTransactionCoreInfo_data_tag) {
		FAIL("Unexpected data");
	}
	return decode_and_store_in_ctx(stream, ctx->dataStr, sizeof(ctx->dataStr));
}

static bool decode_toaddr_callback (pb_istream_t *stream, const pb_field_t *field, void **arg)
{
	uint8_t buf[PUB_ADDR_BYTES_LEN];
	char buf2[BECH32_ENCODE_BUF_LEN];

	CHECK_CANARY;

	if (field->tag != ProtoTransactionCoreInfo_toaddr_tag) {
		FAIL("Unexpected data");
	}

	if (pb_read(stream, (pb_byte_t*) buf, PUB_ADDR_BYTES_LEN)) {
		CHECK_CANARY;
		PRINTF("decoded bytes: 0x%.*h\n", PUB_ADDR_BYTES_LEN, buf);
		// Write data for display.
		if (!bech32_addr_encode(buf2, "zil", buf, PUB_ADDR_BYTES_LEN)) {
			FAIL ("bech32 encoding of sendto address failed");
		}
		CHECK_CANARY;
		if (strlen(buf2) != BECH32_ADDRSTR_LEN) {
			FAIL ("bech32 encoded address of incorrect length");
		}
		assert(sizeof(ctx->toAddrStr) >= BECH32_ADDRSTR_LEN + 1);
		memcpy(ctx->toAddrStr, buf2, BECH32_ADDRSTR_LEN);
		ctx->toAddrStr[BECH32_ADDRSTR_LEN] = '\0';
		CHECK_CANARY;
	} else {
		PRINTF("pb_read failed\n");
		return false;
	}

	CHECK_CANARY;
	return true;
}

static bool decode_amount_gasprice_callback (pb_istream_t *stream, const pb_field_t *field, void **arg)
{
	uint8_t buf[ZIL_AMOUNT_GASPRICE_BYTES];
	char buf2[ZIL_UINT128_BUF_LEN];

	CHECK_CANARY;

	if (field->tag != ByteArray_data_tag) {
		FAIL("Unexpected data");
	}

	if (((int) *arg != ProtoTransactionCoreInfo_amount_tag) &&
	    ((int) *arg != ProtoTransactionCoreInfo_gasprice_tag)) {
		FAIL("Unhandled ByteArray tag");
	}

	if (pb_read(stream, (pb_byte_t*) buf, ZIL_AMOUNT_GASPRICE_BYTES)) {
		CHECK_CANARY;
		PRINTF("decoded bytes: 0x%.*h\n", ZIL_AMOUNT_GASPRICE_BYTES, buf);
		// Write data for display.
		// It is either gasprice or amount. a uint128_t value.
		// Convert to decimal, appending a '\0'.
		// ZIL data is big-endian, we need little-endian here.
		for (int i = 0; i < 4; i++) {
			// The upper 64b and lower 64b themselves aren't swapped, just within them.
			// Upper uint64 is converted to little endian.
			uint8_t t = buf[i];
			buf[i] = buf[7-i];
			buf[7-i] = t;
			// lower uint64 is converted to little endian.
			t = buf[8+i];
			buf[8+i] = buf[15-i];
			buf[15-i] = t;
		}
		CHECK_CANARY;
		// UINT128 can have a maximum of 39 decimal digits. When we convert
		// "Qa" values to "Zil", we may have to append "0." at the start.
		// So a total of 39 + 2 + '\0' = 42.
		if (tostring128((uint128_t*)buf, 10, buf2, sizeof(buf2))) {
			PRINTF("128b to decimal converted value: %s\n", buf2);
			CHECK_CANARY;
			if ((int) *arg != ProtoTransactionCoreInfo_amount_tag) {
				qa_to_zil(buf2, ctx->amountStr, sizeof(ctx->amountStr));
				PRINTF("Amount Qa converted to Zil: %s\n", ctx->amountStr);
			} else {
				qa_to_zil(buf2, ctx->gaspriceStr, sizeof(ctx->gaspriceStr));
				PRINTF("Gasprice Qa converted to Zil: %s\n", ctx->gaspriceStr);
			}
			CHECK_CANARY;
		} else {
			FAIL("Error converting 128b unsigned to decimal");
		}
		CHECK_CANARY;
	} else {
		PRINTF("pb_read failed\n");
		return false;
	}

	CHECK_CANARY;
	return true;
}

// Sign the txn, also deserializes parts of it. May call io_exchange multiple times.
// Output: 1. Display message will be populated in ctx->msg.
//         2. Signature will be populated in ctx->signature.
static bool sign_deserialize_stream(const uint8_t *txn1, int txn1Len, int hostBytesLeft)
{
	// Initialize stream data.
	memcpy(ctx->sd.buf, txn1, txn1Len);
	ctx->sd.nextIdx = 0; ctx->sd.len = txn1Len; ctx->sd.hostBytesLeft = hostBytesLeft;
	assert(hostBytesLeft <= ZIL_MAX_TXN_SIZE - txn1Len);
  // Setup the stream.
	pb_istream_t stream = { istream_callback, &ctx->sd, hostBytesLeft + txn1Len, NULL };

	// Initialize the display messages.
	ctx->toAddrStr[0] = '\0';
	ctx->amountStr[0] = '\0';
	ctx->gaspriceStr[0] = '\0';
	ctx->codeStr[0] = '\0';
	ctx->dataStr[0] = '\0';

	CHECK_CANARY;
	// Initialize schnorr signing, continue with what we have so far.
	deriveAndSignInit(&ctx->ecs, ctx->keyIndex);
	CHECK_CANARY;
	deriveAndSignContinue(&ctx->ecs, txn1, txn1Len);
	CHECK_CANARY;

	// Initialize protobuf Txn structs.
	memset(&ctx->txn, 0, sizeof(ctx->txn));
	// Set callbacks for handling the fields that what we need.
	ctx->txn.toaddr.funcs.decode = decode_toaddr_callback;
	// Since we're using the same callback for amount and gasprice,
	// but the tag for both will be set to "ByteArray", we differentiate with "arg".
	ctx->txn.amount.data.funcs.decode = decode_amount_gasprice_callback;
	ctx->txn.amount.data.arg = (void*)ProtoTransactionCoreInfo_amount_tag;
	ctx->txn.gasprice.data.funcs.decode = decode_amount_gasprice_callback;
	ctx->txn.gasprice.data.arg = (void*)ProtoTransactionCoreInfo_gasprice_tag;
	// Set a decoder for the data and code fields of our transaction.
	ctx->txn.data.funcs.decode = decode_data_callback;
	ctx->txn.code.funcs.decode = decode_code_callback;

	CHECK_CANARY;

	// Start decoding (and signing).
	if (pb_decode(&stream, ProtoTransactionCoreInfo_fields, &ctx->txn)) {
		PRINTF ("pb_decode successful\n");
		deriveAndSignFinish(&ctx->ecs, ctx->keyIndex, ctx->signature, SCHNORR_SIG_LEN_RS);
		PRINTF ("sign_deserialize_stream: signature: 0x%.*h\n", SCHNORR_SIG_LEN_RS, ctx->signature);
	} else {
		PRINTF ("pb_decode failed\n");
		return false;
	}

	CHECK_CANARY;
	return true;
}

void handleSignTxn(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {

	int txnLen, hostBytesLeft;

	static const int dataIndexOffset = 0;      // offset for the key index to use
	static const int dataHostBytesLeftOffset = 4; // offset for integer: is there more data (do io_exhange again)?
	static const int dataTxnLenOffset = 8;     // offset for integer containing length of current txn
	static const int dataOffset = 12;          // offset for actual transaction data.

	// Sanity-check the command length
	if (dataLength < sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t)) {
		THROW(SW_WRONG_DATA_LENGTH);
	}

  // Read the various integers at the beginning.
	ctx->keyIndex = U4LE(dataBuffer, dataIndexOffset);
	// Generate a string for the index.
	snprintf(ctx->indexStr, sizeof(ctx->indexStr), "with Key #%d?", ctx->keyIndex);

	PRINTF("handleSignTxn: keyIndex: %d \n", ctx->keyIndex);
	hostBytesLeft = U4LE(dataBuffer, dataHostBytesLeftOffset);
	PRINTF("handleSignTxn: hostBytesLeft: %d \n", hostBytesLeft);
	txnLen = U4LE(dataBuffer, dataTxnLenOffset);
	PRINTF("handleSignTxn: txnLen: %d\n", txnLen);
	if (dataLength != dataOffset + txnLen) {
		THROW(SW_WRONG_DATA_LENGTH);
	}
	if (txnLen > TXN_BUF_SIZE) {
		FAIL("Cannot handle large data sent from host");
	}

	// Read the (partial) transaction and
	// Sign the txn and get message for confirmation display, all in ctx.
	// Signature will not go back to host until message display + approval.
	if (!sign_deserialize_stream(dataBuffer + dataOffset, txnLen, hostBytesLeft)) {
		FAIL("sign_deserialize_stream failed");
	}

	if (ctx->codeStr[0] == '\0') {
		if (ctx->dataStr[0] == '\0') {
			ux_flow_init(0, ux_signmsg_simple_flow, NULL);
		} else {
			ux_flow_init(0, ux_signmsg_data_flow, NULL);
		}
	} else {
		ux_flow_init(0, ux_signmsg_code_data_flow, NULL);
	}

	// Set the IO_ASYNC_REPLY flag. This flag tells zil_main that we aren't
	// sending data to the computer immediately; we need to wait for a button
	// press first.
	*flags |= IO_ASYNCH_REPLY;
}
