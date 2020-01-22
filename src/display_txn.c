/*
 * Copyright (C) 2020 Zilliqa
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <os.h>

// zilliqa.h must be included before txn_msg.h so as to define PRINTF and FAIL.
#include "zilliqa.h"
#include "txn_msg.h"
#include "display_txn.h"
#include "bech32_addr.h"

int (*bech32_addr_encode_ptr) (
    char *output,
    const char *hrp,
    const uint8_t *prog,
    size_t prog_len
) = bech32_addr_encode;

void (*hex2bin_ptr)(const uint8_t *hexstr, unsigned numhexchars, uint8_t *bin) = hex2bin;

const char* knownContracts[] =
	{
		"zil1pel8hjums9yxuunv75frartsjtudz72ymeq707"
	};

bool isKnownAddr(const char* addr) {

	return true;

	#define n_array(x) (sizeof (x) / sizeof (const char *))

	for (unsigned i = 0; i < n_array(knownContracts); i++) {
		if (!strcmp(addr, knownContracts[i]))
			return true;
	}

	return false;
}

// Append msg to ctx->msg for display. THROW if out of memory.
void append_ctx_msg (signTxnContext_t *ctx, const char* msg, int msg_len)
{
	if (ctx->msgLen + msg_len >= sizeof(ctx->msg)) {
		FAIL("Display memory full");
	}
	
	os_memcpy(ctx->msg + ctx->msgLen, msg, msg_len);
	ctx->msgLen += msg_len;
}

// For known smart contracts, parse ctx->SCMJSON and append to ctx->msg
void display_sc_message(signTxnContext_t *ctx)
{
	// Is this a pre-known smart contract transaction?
	if (!isKnownAddr(ctx->toAddr) || ctx->SCMJSONLen == 0) {
		PRINTF("Not a known smart contract or couldn't parse txn data.\n");
		return;
	}

	PRINTF("Known smart contract, attempting to parse txn data and print details.\n");

	int msg_rem = sizeof(ctx->msg) - ctx->msgLen;
	// Parse the JSON in ctx->SCMJSON and print it in ctx->msg.
	int num_json_chars = process_json
		((const char*) ctx->SCMJSON, ctx->SCMJSONLen, (char*) ctx->msg + ctx->msgLen, msg_rem);
	if (num_json_chars < 0) {
		PRINTF("Writing smart contract txn message details failed\n");
		return;
	}
	ctx->msgLen += num_json_chars;

#ifdef HAVE_BOLOS_APP_STACK_CANARY
	CHECK_CANARY
#endif // HAVE_BOLOS_APP_STACK_CANARY

}
