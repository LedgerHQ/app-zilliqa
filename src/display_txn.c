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

// jsmn.h can be included only once in the entire source.
// If need in multiple places, see the GitHub page for instructions.
#include "jsmn.h"

#include "display_txn.h"

const char *knownContracts[] =
	{
		"zil1pel8hjums9yxuunv75frartsjtudz72ymeq707"
	};

bool isKnownAddr(const char* addr) {

	return true;

	for (int i = 0; i < sizeof(knownContracts); i++) {
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
	if (!isKnownAddr(ctx->toAddr))
		return;
}
