/*******************************************************************************
*
*  (c) 2016 Ledger
*  (c) 2018 Nebulous
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

// This code also serves as a walkthrough for writing your own Ledger Nano S
// app. Begin by reading this file top-to-bottom, and proceed to the next file
// when directed. It is recommended that you install this app on your Nano S
// so that you can see how each section of code maps to real-world behavior.
// This also allows you to experiment by modifying the code and observing the
// effect on the app.
//
// I'll begin by describing the high-level architecture of the app. The entry
// point is this file, main.c, which initializes the app and runs the APDU
// request/response loop. The loop reads APDU packets from the computer, which
// instructs it to run various commands. The Zilliqa app supports three commands,
// each defined in a separate file: getVersion, getPublicKey and signTxn.
// These each make use of Zilliqa-specific functions, which are defined in zilliqa.c.
// Finally, some global variables and helper functions are declared in ux.h.
//
// Each command consists of a command handler and a set of screens. Each
// screen has an associated set of elements that can be rendered, a
// preprocessor that controls which elements are rendered, and a button
// handler that processes user input. The command handler is called whenever
// zil_main receives an APDU requesting that command, and is responsible for
// displaying the first screen of the command. Control flow then moves to the
// button handler for that screen, which selects the next screen to display
// based on which button was pressed. Button handlers are also responsible for
// sending APDU replies back to the computer.
//
// The control flow can be a little confusing to understand, because the
// button handler isn't really on the "main execution path" -- it's only
// called via interrupt, typically while execution is blocked on an
// io_exchange call. (In general, it is instructive to think of io_exchange as
// the *only* call that can block.) io_exchange exchanges APDU packets with
// the computer: first it sends a response packet, then it receives a request
// packet. This ordering may seem strange, but it makes sense when you
// consider that the Nano S has to do work in between receiving a command and
// replying to it. Thus, the packet sent by io_exchange is a *response* to the
// previous request, and the packet received is the next request.
//
// But there's a problem with this flow: in most cases, we can't respond to
// the command request until we've received some user input, e.g. approving a
// signature. If io_exchange is the only call that blocks, how can we tell it
// to wait for user input? The answer is a special flag, IO_ASYNC_REPLY. When
// io_exchange is called with this flag, it blocks, but it doesn't send a
// response; instead, it just waits for a new request. Later on, we make a
// separate call to io_exchange, this time with the IO_RETURN_AFTER_TX flag.
// This call sends the response, and then returns immediately without waiting
// for the next request. Visually, it is clear that these flags have opposite
// effects on io_exchange:
//
//                                      ----Time--->
//    io_exchange:        [---Send Response---|---Wait for Request---]
//    IO_ASYNC_REPLY:                           ^Only do this part^
//    IO_RETURN_AFTER_TX:  ^Only do this part^
//
// So a typical command flow looks something like this. We start in zil_main,
// which is an infinite loop that starts by calling io_exchange. It receives
// an APDU request from the computer and calls the associated command handler.
// The handler displays a screen, e.g. "Generate address?", and sets the
// IO_ASYNC_REPLY flag before returning. Control returns to zil_main, which
// loops around and calls io_exchange again; due to the flag, it now blocks.
// Everything is frozen until the user decides which button to press. When
// they eventually press the "Approve" button, the button handler jumps into
// action. It generates the address, constructs a response APDU containing
// that address, calls io_exchange with IO_RETURN_AFTER_TX, and redisplays the
// main menu. When a new command arrives, it will be received by the blocked
// io_exchange in zil_main.
//
// More complex commands may require multiple requests and responses. There
// are two approaches to handling this. One approach is to treat each command
// handler as a self-contained unit: that is, the main loop should only handle
// the *first* request for a given command. Subsequent requests are handled by
// additional io_exchange calls within the command handler. The other approach
// is to let the main loop handle all requests, and design the handlers so
// that they can "pick up where they left off." Both designs have tradeoffs.
//
// On the other end of the spectrum, there are simple commands that do not
// require any user input. Many Nano S apps have a "getVersion" command that
// replies to the computer with the app's version. In this case, it is
// sufficient for the command handler to prepare the response APDU and allow
// the main loop to send it immediately, without setting IO_ASYNC_REPLY.
//
// The important things to remember are:
// - io_exchange is the only blocking call
// - the main loop invokes command handlers, which display screens and set button handlers
// - button handlers switch between screens and reply to the computer

#include <stdint.h>
#include <stdbool.h>
#include "os_io_seproxyhal.h"
#include "glyphs.h"
#include "zilliqa_ux.h"
#include "zilliqa.h"

// These are global variables declared in ux.h. They can't be defined there
// because multiple files include ux.h; they need to be defined in exactly one
// place. See ux.h for their descriptions.
commandContext global;

ux_state_t G_ux;
bolos_ux_params_t G_ux_params;

static void app_quit(void) {
    // exit app here
    os_sched_exit(-1);
}

#ifdef HAVE_BAGL
UX_STEP_NOCB(
    ux_idle_flow_1_step,
    bn,
    {
      "Application",
      "is ready",
    });
UX_STEP_NOCB(
    ux_idle_flow_2_step,
    bn,
    {
      "Version",
      APPVERSION,
    });
UX_STEP_VALID(
    ux_idle_flow_3_step,
    pb,
    app_quit(),
    {
      &C_icon_dashboard,
      "Quit",
    });

UX_FLOW(ux_idle_flow,
  &ux_idle_flow_1_step,
  &ux_idle_flow_2_step,
  &ux_idle_flow_3_step);

// ui_idle displays the main menu. Note that your app isn't required to use a
// menu as its idle screen; you can define your own completely custom screen.
void ui_idle(void) {
    // reserve a display stack slot if none yet
    if(G_ux.stack_count == 0) {
        ux_stack_push();
    }
    ux_flow_init(0, ux_idle_flow, NULL);
}

#else // HAVE_BAGL

// 'About' menu
static const char* const INFO_TYPES[] = {"Version"};
static const char* const INFO_CONTENTS[] = {APPVERSION};

static bool nav_callback(uint8_t page, nbgl_pageContent_t* content) {
    UNUSED(page);
    content->type = INFOS_LIST;
    content->infosList.nbInfos = 1;
    content->infosList.infoTypes = (const char**) INFO_TYPES;
    content->infosList.infoContents = (const char**) INFO_CONTENTS;
    return true;
}

static void ui_menu_about(void) {
    nbgl_useCaseSettings(APPNAME, 0, 1, false, ui_idle, nav_callback, NULL);
}

void ui_idle(void) {
    nbgl_useCaseHome(APPNAME, &C_zilliqa_stax_64px,
                     NULL, false, ui_menu_about, app_quit);
}

#endif // HAVE_BAGL

// io_exchange_with_code is a helper function for sending response APDUs from
// button handlers. Note that the IO_RETURN_AFTER_TX flag is set. 'tx' is the
// conventional name for the size of the response APDU, i.e. the write-offset
// within G_io_apdu_buffer.
void io_exchange_with_code(uint16_t code, uint16_t tx) {
	G_io_apdu_buffer[tx++] = code >> 8;
	G_io_apdu_buffer[tx++] = code & 0xFF;
	io_exchange(CHANNEL_APDU | IO_RETURN_AFTER_TX, tx);
}

// The APDU protocol uses a single-byte instruction code (INS) to specify
// which command should be executed. We'll use this code to dispatch on a
// table of function pointers.
#define INS_GET_VERSION    0x01
#define INS_GET_PUBLIC_KEY 0x02
#define INS_SIGN_TXN  0x04
#define INS_SIGN_HASH 0x08

// This is the function signature for a command handler. 'flags' and 'tx' are
// out-parameters that will control the behavior of the next io_exchange call
// in zil_main. It's common to set *flags |= IO_ASYNC_REPLY, but tx is
// typically unused unless the handler is immediately sending a response APDU.
typedef void handler_fn_t(uint8_t p1, uint8_t p2, uint8_t *dataBuffer, uint16_t dataLength, volatile unsigned int *flags, volatile unsigned int *tx);

handler_fn_t handleGetVersion;
handler_fn_t handleGetPublicKey;
handler_fn_t handleSignTxn;
handler_fn_t handleSignHash;

static handler_fn_t* lookupHandler(uint8_t ins) {
	switch (ins) {
		case INS_GET_VERSION:    return handleGetVersion;
		case INS_GET_PUBLIC_KEY: return handleGetPublicKey;
		case INS_SIGN_TXN:  return handleSignTxn;
		case INS_SIGN_HASH: return handleSignHash;
		default:                 return NULL;
	}
}

// This is the main loop that reads and writes APDUs. It receives request
// APDUs from the computer, looks up the corresponding command handler, and
// calls it on the APDU payload. Then it loops around and calls io_exchange
// again. The handler may set the 'flags' and 'tx' variables, which affect the
// subsequent io_exchange call. The handler may also throw an exception, which
// will be caught, converted to an error code, appended to the response APDU,
// and sent in the next io_exchange call.
static void zil_main(void) {

	volatile unsigned int rx = 0;
	volatile unsigned int tx = 0;
	volatile unsigned int flags = 0;

	// Exchange APDUs until EXCEPTION_IO_RESET is thrown.
	for (;;) {
		volatile unsigned short sw = 0;

		// The Ledger SDK implements a form of exception handling. In addition
		// to explicit THROWs in user code, syscalls (prefixed with os_ or
		// cx_) may also throw exceptions.
		//
		// In zil_main, this TRY block serves to catch any thrown exceptions
		// and convert them to response codes, which are then sent in APDUs.
		// However, EXCEPTION_IO_RESET will be re-thrown and caught by the
		// "true" main function defined at the bottom of this file.
		BEGIN_TRY {
			TRY {
				rx = tx;
				tx = 0; // ensure no race in CATCH_OTHER if io_exchange throws an error
				rx = io_exchange(CHANNEL_APDU | flags, rx);

                PLOC();

                flags = 0;

				// No APDU received; trigger a reset.
				if (rx == 0) {
					THROW(EXCEPTION_IO_RESET);
				}
				// Minimal APDU length
				if (rx < OFFSET_CDATA) {
					THROW(SW_WRONG_DATA_LENGTH);
				}
				// APDU length and LC field consistency
				if (rx - OFFSET_CDATA != G_io_apdu_buffer[OFFSET_LC]) {
					THROW(SW_WRONG_DATA_LENGTH);
				}
				// Malformed APDU.
				if (G_io_apdu_buffer[OFFSET_CLA] != CLA) {
					THROW(SW_CLA_NOT_SUPPORTED);
				}
				// Lookup and call the requested command handler.
				handler_fn_t *handlerFn = lookupHandler(G_io_apdu_buffer[OFFSET_INS]);
				if (!handlerFn) {
					THROW(SW_INS_NOT_SUPPORTED);
				}
				INIT_CANARY;
				handlerFn(G_io_apdu_buffer[OFFSET_P1], G_io_apdu_buffer[OFFSET_P2],
				          G_io_apdu_buffer + OFFSET_CDATA, G_io_apdu_buffer[OFFSET_LC], &flags, &tx);
			}
			CATCH(EXCEPTION_IO_RESET) {
                PLOC();
				THROW(EXCEPTION_IO_RESET);
			}
			CATCH_OTHER(e) {
				// Convert the exception to a response code. All error codes
				// start with 6, except for 0x9000, which is a special
				// "success" code. Every APDU payload should end with such a
				// code, even if no other data is sent.
				//
				// If the first byte is not a 6, mask it with 0x6800 to
				// convert it to a proper error code. I'm not totally sure why
				// this is done; perhaps to handle single-byte exception
				// codes?
                PLOC();
                PRINTF("e:%d\n", e);
				switch (e & 0xF000) {
				case 0x6000:
				case 0x9000:
					sw = e;
					break;
				default:
					sw = 0x6800 | (e & 0x7FF);
					break;
				}
				G_io_apdu_buffer[tx++] = sw >> 8;
				G_io_apdu_buffer[tx++] = sw & 0xFF;
			}
			FINALLY {
			}
		}
		END_TRY;
	}
}


// Everything below this point is Ledger magic. And the magic isn't well-
// documented, so if you want to understand it, you'll need to read the
// source, which you can find in the nanos-secure-sdk repo. Fortunately, you
// don't need to understand any of this in order to write an app.
//
// Next, we'll look at how the various commands are implemented. We'll start
// with the simplest command, signHash.c.

#ifdef HAVE_BAGL
// override point, but nothing more to do
void io_seproxyhal_display(const bagl_element_t *element) {
	io_seproxyhal_display_default(element);
}
#endif  // HAVE_BAGL

unsigned char G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];

unsigned char io_event(unsigned char channel) {
	UNUSED(channel);
	// can't have more than one tag in the reply, not supported yet.
	switch (G_io_seproxyhal_spi_buffer[0]) {
	case SEPROXYHAL_TAG_BUTTON_PUSH_EVENT:
#ifdef HAVE_BAGL
		UX_BUTTON_PUSH_EVENT(G_io_seproxyhal_spi_buffer);
#endif  // HAVE_BAGL
		break;
	case SEPROXYHAL_TAG_STATUS_EVENT:
		if (G_io_apdu_media == IO_APDU_MEDIA_USB_HID &&
			!(U4BE(G_io_seproxyhal_spi_buffer, 3) &
			  SEPROXYHAL_TAG_STATUS_EVENT_FLAG_USB_POWERED)) {
			THROW(EXCEPTION_IO_RESET);
		}
		/* fallthrough */
	case SEPROXYHAL_TAG_DISPLAY_PROCESSED_EVENT:
#ifdef HAVE_BAGL
		UX_DISPLAYED_EVENT({});
#endif  // HAVE_BAGL
#ifdef HAVE_NBGL
		UX_DEFAULT_EVENT();
#endif  // HAVE_NBGL
		break;
#ifdef HAVE_NBGL
	case SEPROXYHAL_TAG_FINGER_EVENT:
		UX_FINGER_EVENT(G_io_seproxyhal_spi_buffer);
		break;
#endif  // HAVE_NBGL
	case SEPROXYHAL_TAG_TICKER_EVENT:
		UX_TICKER_EVENT(G_io_seproxyhal_spi_buffer, {});
		break;
	default:
		UX_DEFAULT_EVENT();
		break;
	}

	// close the event if not done previously (by a display or whatever)
	if (!io_seproxyhal_spi_is_status_sent()) {
		io_seproxyhal_general_status();
	}

	// command has been processed, DO NOT reset the current APDU transport
	return 1;
}

unsigned short io_exchange_al(unsigned char channel, unsigned short tx_len) {
	switch (channel & ~(IO_FLAGS)) {
	case CHANNEL_KEYBOARD:
		break;
	// multiplexed io exchange over a SPI channel and TLV encapsulated protocol
	case CHANNEL_SPI:
		if (tx_len) {
			io_seproxyhal_spi_send(G_io_apdu_buffer, tx_len);
			if (channel & IO_RESET_AFTER_REPLIED) {
				reset();
			}
			return 0; // nothing received from the master so far (it's a tx transaction)
		} else {
			return io_seproxyhal_spi_recv(G_io_apdu_buffer, sizeof(G_io_apdu_buffer), 0);
		}
	default:
		THROW(INVALID_PARAMETER);
	}
	return 0;
}

static void app_exit(void) {
	BEGIN_TRY_L(exit) {
		TRY_L(exit) {
			os_sched_exit(-1);
		}
		FINALLY_L(exit) {
		}
	}
	END_TRY_L(exit);
}

__attribute__((section(".boot"))) int main(void) {
	// exit critical section
	__asm volatile("cpsie i");

	for (;;) {
		UX_INIT();
		os_boot();
		BEGIN_TRY {
			TRY {
				io_seproxyhal_init();
#ifdef HAVE_BLE
				// grab the current plane mode setting
				G_io_app.plane_mode = os_setting_get(OS_SETTING_PLANEMODE, NULL, 0);
#endif // HAVE_BLE
				USB_power(0);
				USB_power(1);
				ui_idle();
#ifdef HAVE_BLE
				BLE_power(0, NULL);
				BLE_power(1, "Nano X");
#endif // HAVE_BLE
				zil_main();
			}
			CATCH(EXCEPTION_IO_RESET) {
				// reset IO and UX before continuing
				continue;
			}
			CATCH_ALL {
				break;
			}
			FINALLY {
			}
		}
		END_TRY;
	}
	app_exit();
	return 0;
}
