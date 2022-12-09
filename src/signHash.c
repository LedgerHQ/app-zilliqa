// This file contains the implementation of the signHash command. The files
// for the other commands will have the same basic structure: A set of screens
// (comprising the screen elements, preprocessor, and button handler) followed
// by the command handler itself.
//
// A high-level description of signHash is as follows. The user initiates the
// command on their computer, specifying the hash they would like to sign and
// the key they would like to sign with. The command handler then displays the
// hash on the device and asks the user to compare it to the hash shown on the
// computer. The user can press the left and right buttons to scroll through
// the hash. When the user finishes comparing, they press both buttons to
// proceed to the next screen, which asks the user to approve or deny signing
// the hash. If the user presses the left button, the action is denied and a
// rejection code is sent to the computer. If they press the right button, the
// action is approved and the requested signature is computed and sent to the
// computer. In either case, the command ends by returning to the main screen.
//
// Keep this description in mind as you read through the implementation.

#include <stdint.h>
#include <stdbool.h>
#include "os.h"
#include "os_io_seproxyhal.h"

#include "zilliqa.h"
#include "zilliqa_ux.h"

// Get a pointer to signHash's state variables. This is purely for
// convenience, so that we can refer to these variables concisely from any
// signHash-related function.
static signHashContext_t * const ctx = &global.signHashContext;

static void do_approve(const bagl_element_t *e)
{
		// Derive the secret key and sign the hash, storing the signature in
		// the APDU buffer.
		assert(SHA256_HASH_LEN == sizeof(ctx->hash));
		deriveAndSign(G_io_apdu_buffer, SCHNORR_SIG_LEN_RS,
									ctx->keyIndex, ctx->hash, SHA256_HASH_LEN);
		// Send the data in the APDU buffer, which is a 64 byte signature.
		assert(IO_APDU_BUFFER_SIZE >= SCHNORR_SIG_LEN_RS);
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
    ux_signhash_flow_1_step,
    pnn,
    {
      &C_icon_certificate,
      "Sign SHA256 Hash",
      ctx->indexStr,
    });
UX_FLOW_DEF_NOCB(
    ux_signhash_flow_2_step,
    bnnn_paging,
    {
      .title = "Hash",
      .text = ctx->hexHash,
    });
UX_FLOW_DEF_VALID(
    ux_signhash_flow_3_step,
    pn,
    do_approve(NULL),
    {
      &C_icon_validate_14,
      "Sign",
    });
UX_FLOW_DEF_VALID(
    ux_signhash_flow_4_step,
    pn,
    do_reject(NULL),
    {
      &C_icon_crossmark,
      "Cancel",
    });

UX_FLOW(ux_signhash_flow,
  &ux_signhash_flow_1_step,
  &ux_signhash_flow_2_step,
  &ux_signhash_flow_3_step,
  &ux_signhash_flow_4_step
);

// handleSignHash is the entry point for the signHash command. Like all
// command handlers, it is responsible for reading command data from
// dataBuffer, initializing the command context, and displaying the first
// screen of the command.
void handleSignHash(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx) {
	if (dataLength != sizeof(uint32_t) + sizeof(ctx->hash)) {
		FAIL("Incorrect dataLength calling handleSignHash");
	}

	// Read the index of the signing key. U4LE is a helper macro for
	// converting a 4-byte buffer to a uint32_t.
	ctx->keyIndex = U4LE(dataBuffer, 0);
	// Generate a string for the index.
	snprintf(ctx->indexStr, sizeof(ctx->indexStr), "with Key #%d?", ctx->keyIndex);

	// Read the hash.
	memmove(ctx->hash, dataBuffer+4, sizeof(ctx->hash));
	// Prepare to display the comparison screen by converting the hash to hex
	snprintf(ctx->hexHash, sizeof(ctx->hexHash), "%.*h", sizeof(ctx->hash), ctx->hash);
	PRINTF("hash:    %.*H \n", 32, ctx->hash);
	PRINTF("hexHash: %.*H \n", 64, ctx->hexHash);

	ux_flow_init(0, ux_signhash_flow, NULL);

	// Set the IO_ASYNC_REPLY flag. This flag tells zil_main that we aren't
	// sending data to the computer immediately; we need to wait for a button
	// press first.
	*flags |= IO_ASYNCH_REPLY;
}

// Now that we've seen the individual pieces, we can construct a full picture
// of what the signHash command looks like.
//
// The command begins when zil_main reads an APDU packet from the computer
// with INS == INS_SIGN_HASH. zil_main looks up the appropriate handler,
// handleSignHash, and calls it. handleSignHash reads the command data,
// prepares and displays the comparison screen, and sets the IO_ASYNC_REPLY
// flag. Control returns to zil_main, which blocks when it reaches the
// io_exchange call.
//
// UX_DISPLAY was called with the ui_prepro_signHash_compare preprocessor, so
// that preprocessor is now called each time the compare screen is rendered.
// Since we are initially displaying the beginning of the hash, the
// preprocessor hides the left arrow. The user presses and holds the right
// button, which triggers the button handler to advance the displayIndex every
// 100ms. Each advance requires redisplaying the screen via UX_REDISPLAY(),
// and thus rerunning the preprocessor. As soon as the right button is
// pressed, the preprocessor detects that text has scrolled off the left side
// of the screen, so it unhides the left arrow; when the end of the hash is
// reached, it hides the right arrow.
//
// When the user has finished comparing the hashes, they press both buttons
// together, triggering ui_signHash_compare_button to prepare the approval
// screen and call UX_DISPLAY on ui_signHash_approve. A NULL preprocessor is
// specified for this screen, since we don't need to filter out any of its
// elements. We'll assume that the user presses the 'approve' button, causing
// the button handler to place the hash in G_io_apdu_buffer and call
// io_exchange_with_code, which sends the response APDU to the computer with
// the IO_RETURN_AFTER_TX flag set. The button handler then calls ui_idle,
// thus returning to the main menu.
//
// This completes the signHash command. Back in zil_main, io_exchange is still
// blocked, waiting for the computer to send a new request APDU. For the next
// section of this walkthrough, we will assume that the next APDU requests the
// getPublicKey command, so proceed to getPublicKey.c.
