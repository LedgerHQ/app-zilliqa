// This file contains the implementation of the getPublicKey command. It is
// broadly similar to the signHash command, but with a few new features. Since
// much of the code is the same, expect far fewer comments.
//
// A high-level description of getPublicKey is as follows. The user initiates
// the command on their computer by requesting the generation of a specific
// public key. The command handler then displays a screen asking the user to
// confirm the action. If the user presses the 'approve' button, the requested
// key is generated, sent to the computer, and displayed on the device. The
// user may then visually compare the key shown on the device to the key
// received by the computer. Augmenting this, the user may optionally request
// that an address be generated from the public key, in which case this
// address is displayed instead of the public key. A final two-button press
// returns the user to the main screen.
//
// Note that the order of the getPublicKey screens is the reverse of signHash:
// first approval, then comparison.
//
// Keep this description in mind as you read through the implementation.

#include <stdint.h>
#include <stdbool.h>
#include "os.h"
#include "os_io_seproxyhal.h"

#include "glyphs.h"
#include "zilliqa_ux.h"
#include "zilliqa.h"
#include "bech32_addr.h"

// Get a pointer to getPublicKey's state variables.
static getPublicKeyContext_t * const ctx = &global.getPublicKeyContext;

// 1. Derive the public key and address and store them in the APDU
// buffer. (For simplicity, we always send both, regardless of which
// one the user requested.)
// 2. Fill ctx->fullStr with the display string.
static int prepareZilPubKeyAddr()
{
    // The response APDU will contain multiple objects, which means we need to
    // remember our offset within G_io_apdu_buffer. By convention, the offset
    // variable is named 'tx' and begins at 0.
    uint16_t tx = 0;
    cx_ecfp_public_key_t publicKey;

    // 1. Generate public key
    deriveZilPubKey(ctx->keyIndex, &publicKey);
    assert(publicKey.W_len == PUBLIC_KEY_BYTES_LEN);
    memmove(G_io_apdu_buffer + tx, publicKey.W, publicKey.W_len);
    tx += publicKey.W_len;
    // 2. Generate address from public key.
    uint8_t bytesAddr[PUB_ADDR_BYTES_LEN];
    pubkeyToZilAddress(bytesAddr, &publicKey);
    // We have the address bytes, convert that to a null-terminated bech32 string.
    // 73 is the max size needed, as per bech32_addr_encode spec. 3 more for "zil".
    char bech32Str[73+3];
    bech32_addr_encode(bech32Str, "zil", bytesAddr, PUB_ADDR_BYTES_LEN);
    // Copy over the bech32 string to the apdu buffer for exchange.
    memcpy(G_io_apdu_buffer + tx, bech32Str, BECH32_ADDRSTR_LEN);
    tx += BECH32_ADDRSTR_LEN;

    PRINTF("Public Key: %.*h\n", publicKey.W_len, G_io_apdu_buffer);
    PRINTF("Address: %s\n", bech32Str);

    //  ctx->fullStr will contain the final text for display.
    if (ctx->genAddr) {
        // The APDU buffer contains printable bech32 string.
        memcpy(ctx->fullStr, G_io_apdu_buffer + publicKey.W_len, BECH32_ADDRSTR_LEN);
        assert(sizeof(ctx->fullStr) >= BECH32_ADDRSTR_LEN + 1);
        ctx->fullStr[BECH32_ADDRSTR_LEN] = '\0';
    } else {
        // The APDU buffer contains the raw bytes of the public key.
        // So, first we need to convert to a human-readable form.
        snprintf(ctx->fullStr, sizeof(ctx->fullStr), "%.*h", publicKey.W_len, G_io_apdu_buffer);
    }

    return tx;
}

static void do_approve(void)
{
    // tx must ideally be gotten from prepareZilPubKeyAddr(),
    // but our flow makes it a bit difficult. So this is a hack.
    int tx = PUBLIC_KEY_BYTES_LEN + BECH32_ADDRSTR_LEN;
    io_exchange_with_code(SW_OK, tx);
#ifdef HAVE_BAGL
    ui_idle();
#else
    nbgl_useCaseStatus("ADDRESS\nVERIFIED", true, ui_idle);
#endif
}

static void do_reject(void)
{
    io_exchange_with_code(SW_USER_REJECTED, 0);
#ifdef HAVE_BAGL
    ui_idle();
#else
    nbgl_useCaseStatus("Address verification\ncancelled", false, ui_idle);
#endif
}

#ifdef HAVE_BAGL
UX_STEP_NOCB(
    ux_display_public_flow_1_step,
    pnn,
    {
      &C_icon_eye,
      ctx->typeStr,
      ctx->keyStr,
    });
UX_STEP_NOCB(
    ux_display_public_flow_2_step,
    bnnn_paging,
    {
      .title = "Value",
      .text = ctx->fullStr,
    });
UX_STEP_VALID(
    ux_display_public_flow_3_step,
    pb,
    do_approve(),
    {
      &C_icon_validate_14,
      "Approve",
    });
UX_STEP_VALID(
    ux_display_public_flow_4_step,
    pb,
    do_reject(),
    {
      &C_icon_crossmark,
      "Reject",
    });

UX_FLOW(ux_display_public_flow,
  &ux_display_public_flow_1_step,
  &ux_display_public_flow_2_step,
  &ux_display_public_flow_3_step,
  &ux_display_public_flow_4_step);

void ui_display_public_key_flow(void) {
    // Prepare the approval screen, filling in the header and body text.
    if (ctx->genAddr) {
        strlcpy(ctx->typeStr, "Generate Address", sizeof(ctx->typeStr));
    }
    else {
        strlcpy(ctx->typeStr, "Generate Public", sizeof(ctx->typeStr));
    }
    snprintf(ctx->keyStr, sizeof(ctx->keyStr), "Key #%d?", ctx->keyIndex);

    ux_flow_init(0, ux_display_public_flow, NULL);
}

#else // HAVE_BAGL

static void address_verification_cancelled(void) {
    do_reject();
}

static void display_address_callback(bool confirm) {
    if (confirm) {
        do_approve();
    } else {
        address_verification_cancelled();
    }
}

// called when tapping on review start page to actually display address
static void display_addr(void) {
    nbgl_useCaseAddressConfirmation(ctx->fullStr,
                                    &display_address_callback);
}

void ui_display_public_key_flow(void) {
    if (ctx->genAddr) {
        strlcpy(ctx->typeStr, "Verify Zilliqa\n Address", sizeof(ctx->typeStr));
    }
    else {
        strlcpy(ctx->typeStr, "Verify Zilliqa\n Public Key", sizeof(ctx->typeStr));
    }
    snprintf(ctx->keyStr, sizeof(ctx->keyStr), "Using key index %d", ctx->keyIndex);


    nbgl_useCaseReviewStart(&C_zilliqa_stax_64px,
                            ctx->typeStr, ctx->keyStr, "Cancel",
                            display_addr, address_verification_cancelled);
}
#endif // HAVE_BAGL

// These are APDU parameters that control the behavior of the getPublicKey
// command.
#define P2_DISPLAY_PUBKEY  0x00
#define P2_DISPLAY_ADDRESS 0x01

// handleGetPublicKey is the entry point for the getPublicKey command. It
// reads the command parameters, prepares and displays the approval screen,
// and sets the IO_ASYNC_REPLY flag.
void handleGetPublicKey(uint8_t p1,
                        uint8_t p2,
                        uint8_t *dataBuffer,
                        uint16_t dataLength,
                        volatile unsigned int *flags,
                        volatile unsigned int *tx) {
    UNUSED(p1);
    UNUSED(tx);
    // Sanity-check the command parameters.
    if ((p2 != P2_DISPLAY_ADDRESS) && (p2 != P2_DISPLAY_PUBKEY)) {
        // Although THROW is technically a general-purpose exception
        // mechanism, within a command handler it is basically just a
        // convenient way of bailing out early and sending an error code to
        // the computer. The exception will be caught by zil_main, which
        // appends the code to the response APDU and sends it, much like
        // io_exchange_with_code. THROW should not be called from
        // preprocessors or button handlers.
        THROW(SW_INVALID_PARAM);
    }

    // Sanity-check the command length
    if (dataLength != sizeof(uint32_t)) {
        THROW(SW_WRONG_DATA_LENGTH);
    }

    // Read the key index from dataBuffer and set the genAddr flag according
    // to p2.
    ctx->keyIndex = U4LE(dataBuffer, 0);
    ctx->genAddr = (p2 == P2_DISPLAY_ADDRESS);

    prepareZilPubKeyAddr();
    ui_display_public_key_flow();

    *flags |= IO_ASYNCH_REPLY;
}

// Having previously read through signHash.c, getPublicKey.c shouldn't be too
// difficult to make sense of. We'll move on to the last (and most complex)
// command file in the walkthrough, calcTxnHash.c. Hold onto your hat!
