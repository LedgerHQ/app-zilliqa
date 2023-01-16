#include <stdint.h>
#include <stdbool.h>
#include <os.h>
#include <os_io_seproxyhal.h>
#include "zilliqa.h"
#include "ux.h"

// handleGetVersion is the entry point for the getVersion command. It
// unconditionally sends the app version.
void handleGetVersion(uint8_t ZILLIQA_UNUSED p1, uint8_t ZILLIQA_UNUSED p2, uint8_t * ZILLIQA_UNUSED dataBuffer, uint16_t ZILLIQA_UNUSED dataLength, volatile unsigned int * ZILLIQA_UNUSED flags, volatile unsigned int * ZILLIQA_UNUSED tx) {
	G_io_apdu_buffer[0] = APPVERSION[0] - '0';
	G_io_apdu_buffer[1] = APPVERSION[2] - '0';
	G_io_apdu_buffer[2] = APPVERSION[4] - '0';
	io_exchange_with_code(SW_OK, 3);
}
