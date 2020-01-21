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

#include <stdbool.h>

#include "glyphs.h"
#include "ux.h"

// Append msg to ctx->msg for display. THROW if out of memory.
void append_ctx_msg (signTxnContext_t *ctx, const char* msg, int msg_len);

// For known smart contracts, parse ctx->SCMJSON and append to ctx->msg
void display_sc_message(signTxnContext_t *ctx);
