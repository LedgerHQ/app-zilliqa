#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <os.h>
#include <cx.h>
#include "schnorr.h"
#include "zilliqa.h"
#include "qatozil.h"

#define KEY_SEED_LEN 32

void getKeySeed(uint8_t* keySeed, uint32_t index) {

    // bip32 path for 44'/313'/n'/0'/0'
    // 313 0x80000139 ZIL Zilliqa
    uint32_t bip32Path[] = {44 | 0x80000000,
                            313 | 0x80000000,
                            index | 0x80000000,
                            0x80000000,
                            0x80000000};

    os_perso_derive_node_bip32(CX_CURVE_SECP256K1, bip32Path, 5, keySeed, NULL);
    PRINTF("keySeed: %.*H \n", KEY_SEED_LEN, keySeed);
}

void compressPubKey(cx_ecfp_public_key_t *publicKey) {
    // Uncompressed key has 0x04 + X (32 bytes) + Y (32 bytes).
    if (publicKey->W_len != 65 || publicKey->W[0] != 0x04) {
        PRINTF("compressPubKey: Input public key is incorrect\n");
        THROW(SW_INVALID_PARAM);
    }

    // check if Y is even or odd. Assuming big-endian, just check the last byte.
    if (publicKey->W[64] % 2 == 0) {
        // Even
        publicKey->W[0] = 0x02;
    } else {
        // Odd
        publicKey->W[0] = 0x03;
    }

    publicKey->W_len = PUBLIC_KEY_BYTES_LEN;
    PLOC();
}

void deriveZilPubKey(uint32_t index,
                      cx_ecfp_public_key_t *publicKey) {
    cx_ecfp_private_key_t pk;

    uint8_t keySeed[KEY_SEED_LEN];
    getKeySeed(keySeed, index);

    cx_ecfp_init_private_key(CX_CURVE_SECP256K1, keySeed, 32, &pk);

    assert (publicKey);
    cx_ecfp_init_public_key(CX_CURVE_SECP256K1, NULL, 0, publicKey);
    cx_ecfp_generate_pair(CX_CURVE_SECP256K1, publicKey, &pk, 1);
    PRINTF("publicKey:\n %.*H \n\n", publicKey->W_len, publicKey->W);

    compressPubKey(publicKey);

    memset(keySeed, 0, sizeof(keySeed));
    memset(&pk, 0, sizeof(pk));
    PLOC();
}

void deriveAndSign(uint8_t *dst, uint32_t dst_len, uint32_t index, const uint8_t *msg, unsigned int msg_len) {
    PRINTF("deriveAndSign: index: %d\n", index);
    PRINTF("deriveAndSign: msg: %.*H \n", msg_len, msg);

    uint8_t keySeed[KEY_SEED_LEN];
    getKeySeed(keySeed, index);

    cx_ecfp_private_key_t privateKey;
    cx_ecfp_init_private_key(CX_CURVE_SECP256K1, keySeed, 32, &privateKey);
    PRINTF("deriveAndSign: privateKey: %.*H \n", privateKey.d_len, privateKey.d);

    if (dst_len != SCHNORR_SIG_LEN_RS)
        THROW (INVALID_PARAMETER);

    zil_ecschnorr_sign(&privateKey, msg, msg_len, dst, dst_len);
    PRINTF("deriveAndSign: signature: %.*H\n", SCHNORR_SIG_LEN_RS, dst);

    // Erase private keys for better security.
    memset(keySeed, 0, sizeof(keySeed));
    memset(&privateKey, 0, sizeof(privateKey));
}

void deriveAndSignInit(zil_ecschnorr_t *T, uint32_t index)
{
    PRINTF("deriveAndSignInit: index: %d\n", index);

    uint8_t keySeed[KEY_SEED_LEN];

    CHECK_CANARY;
    getKeySeed(keySeed, index);
    cx_ecfp_private_key_t privateKey;
    cx_ecfp_init_private_key(CX_CURVE_SECP256K1, keySeed, 32, &privateKey);
    PRINTF("deriveAndSignInit: privateKey: %.*H \n", privateKey.d_len, privateKey.d);

    CHECK_CANARY;
    zil_ecschnorr_sign_init (T, &privateKey);
	CHECK_CANARY;

    // Erase private keys for better security.
    memset(keySeed, 0, sizeof(keySeed));
    memset(&privateKey, 0, sizeof(privateKey));
}

void deriveAndSignContinue(zil_ecschnorr_t *T, const uint8_t *msg, unsigned int msg_len)
{
    PRINTF("deriveAndSignContinue: msg: %.*H \n", msg_len, msg);

    CHECK_CANARY;
    zil_ecschnorr_sign_continue(T, msg, msg_len);
    CHECK_CANARY;
}

int deriveAndSignFinish(zil_ecschnorr_t *T, uint32_t index, unsigned char *dst, unsigned int dst_len)
{
    PRINTF("deriveAndSignFinish: index: %d\n", index);

    uint8_t keySeed[KEY_SEED_LEN];

    CHECK_CANARY;
    getKeySeed(keySeed, index);
    cx_ecfp_private_key_t privateKey;
    cx_ecfp_init_private_key(CX_CURVE_SECP256K1, keySeed, 32, &privateKey);
    PRINTF("deriveAndSignFinish: privateKey: %.*H \n", privateKey.d_len, privateKey.d);

    if (dst_len != SCHNORR_SIG_LEN_RS)
        THROW (INVALID_PARAMETER);

    CHECK_CANARY;
    uint32_t s = zil_ecschnorr_sign_finish(T, &privateKey, dst, dst_len);
    PRINTF("deriveAndSignFinish: signature: %.*H\n", SCHNORR_SIG_LEN_RS, dst);

    // Erase private keys for better security.
    memset(keySeed, 0, sizeof(keySeed));
    memset(&privateKey, 0, sizeof(privateKey));
    CHECK_CANARY;

    return s;
}

void pubkeyToZilAddress(uint8_t *dst, cx_ecfp_public_key_t *publicKey) {
    // 3. Apply SHA2-256 to the pub key
    uint8_t digest[SHA256_HASH_LEN];
    cx_hash_sha256(publicKey->W, publicKey->W_len, digest, SHA256_HASH_LEN);
    PRINTF("sha256: %.*H\n", SHA256_HASH_LEN, digest);

    // LSB 20 bytes of the hash is our address.
    for (unsigned i = 0; i < 20; i++) {
        dst[i] = digest[i+12];
    }
}

void print_available_stack()
{
    uint32_t stack_top = 0;
    PRINTF("Stack remaining: CUR_STACK_ADDR: 0x%p, STACK_LIMIT: 0x%p, Available: %d\n", 
        &stack_top, &_stack, ((uintptr_t)&stack_top) - ((uintptr_t)&_stack));
}
