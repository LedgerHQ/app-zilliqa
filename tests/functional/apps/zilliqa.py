from contextlib import contextmanager
from enum import IntEnum
from typing import Generator
from struct import pack
from pyzil.crypto.schnorr import verify
from bip_utils.addr import ZilAddrEncoder

from ragger.backend.interface import BackendInterface, RAPDU
from ragger.utils import split_message


class INS(IntEnum):
    INS_GET_VERSION = 0x01
    INS_GET_PUBLIC_KEY = 0x02
    INS_SIGN_TXN = 0x04
    INS_SIGN_HASH = 0x08


CLA = 0xE0

P2_DISPLAY_PUBKEY = 0x00
P2_DISPLAY_ADDRESS = 0x01
P2_DISPLAY_NONE = 0x02

STREAM_LEN = 16  # Stream in batches of STREAM_LEN bytes each.

STATUS_OK = 0x9000


class ErrorType:
    SW_USER_REJECTED = 0x6985
    SW_INVALID_PARAM = 0x6B01
    SW_INS_NOT_SUPPORTED = 0x6D00
    SW_CLA_NOT_SUPPORTED = 0x6E00


class ZilliqaClient:
    def __init__(self, backend: BackendInterface):
        self._backend = backend

    def send_get_version(self) -> (int, int, int):
        rapdu: RAPDU = self._backend.exchange(CLA, INS.INS_GET_VERSION, 0, 0, b"")
        response = rapdu.data
        # response = LEDGER_MAJOR_VERSION (1) ||
        #            LEDGER_MINOR_VERSION (1) ||
        #            LEDGER_PATCH_VERSION (1)
        assert len(response) == 3

        major = int(response[0])
        minor = int(response[1])
        patch = int(response[2])
        return (major, minor, patch)

    def compute_adress_from_public_key(self, public_key: bytes) -> str:
        return ZilAddrEncoder.EncodeKey(public_key)

    def parse_get_public_key_response(self, response: bytes) -> (bytes, str, bytes):
        # response = public_key (33) ||
        #            address (42)
        assert len(response) == 33 + 42
        public_key: bytes = response[:33]
        address: str = response[33:33+42].decode("ascii")
        assert self.compute_adress_from_public_key(public_key) == address
        return public_key, address

    @contextmanager
    def send_async_get_public_key(self, index: int,
                                  disp_addr: bool) -> Generator[None, None, None]:
        p1 = 0
        p2 = P2_DISPLAY_ADDRESS if disp_addr else P2_DISPLAY_PUBKEY

        payload = pack("<I", index)
        with self._backend.exchange_async(CLA, INS.INS_GET_PUBLIC_KEY,
                                          p1, p2, payload):
            yield

    def send_get_public_key_non_confirm(self, index: int) -> RAPDU:
        p1 = 0
        p2 = P2_DISPLAY_NONE

        payload = pack("<I", index)
        return self._client.exchange(CLA, INS.INS_GET_PUBLIC_KEY,
                                     p1, p2, payload)

    @contextmanager
    def send_async_sign_transaction_message(self,
                                            index: int,
                                            transaction: bytes) -> Generator[None, None, None]:

        chunks = split_message(transaction, STREAM_LEN)
        total_size = len(transaction)
        sent_size = 0

        for chunk in chunks:
            chunk_size = len(chunk)

            payload = b""
            if sent_size == 0:
                payload += pack("<I", index)
            payload += pack("<I", total_size - sent_size - chunk_size)
            payload += pack("<I", chunk_size)
            payload += chunk

            sent_size += chunk_size
            if sent_size < total_size:
                self._backend.exchange(CLA, INS.INS_SIGN_TXN, 0, 0, payload)
            else:
                with self._backend.exchange_async(CLA, INS.INS_SIGN_TXN, 0, 0, payload):
                    yield

    @contextmanager
    def send_async_sign_hash_message(self,
                                     index: int,
                                     hash_bytes: bytes) -> Generator[None, None, None]:

        payload = pack("<I", index)
        payload += hash_bytes
        with self._backend.exchange_async(CLA, INS.INS_SIGN_HASH, 0, 0, payload):
            yield

    def verify_signature(self, message, response, public_key):
        assert verify(message, response, public_key)

    def get_async_response(self) -> RAPDU:
        return self._backend.last_async_response
