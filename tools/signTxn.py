#!/usr/bin/env python3

import sys
import argparse

from pathlib import Path

from ragger.backend import LedgerCommBackend

REPO_ROOT_DIRECTORY = Path(__file__).parent
ZILLIQA_LIB_DIRECTORY = (REPO_ROOT_DIRECTORY / "../tests/functional/apps").resolve().as_posix()
sys.path.append(ZILLIQA_LIB_DIRECTORY)
from zilliqa import ZilliqaClient
from txn_pb2 import ByteArray, ProtoTransactionCoreInfo


def main(args):
    senderpubkey = ByteArray(data=bytes.fromhex("0205273e54f262f8717a687250591dcfb5755b8ce4e3bd340c7abefd0de1276574"))
    toaddr = bytes.fromhex("8AD0357EBB5515F694DE597EDA6F3F6BDBAD0FD9")
    amount = ByteArray(data=(100).to_bytes(16, byteorder='big'))
    gasprice = ByteArray(data=(1000000000).to_bytes(16, byteorder='big'))
    transaction = ProtoTransactionCoreInfo(
        version=65537,
        nonce=13,
        toaddr=toaddr,
        senderpubkey=senderpubkey,
        amount=amount,
        gasprice=gasprice,
        gaslimit=1
    ).SerializeToString()

    with LedgerCommBackend(None, interface="hid") as backend:
        zilliqa = ZilliqaClient(backend)

        with zilliqa.send_async_sign_transaction_message(args.index, transaction):
            print("Please accept the request on the device")

        result = zilliqa.get_async_response().data
        print("Response: " + result.hex())
        print("Length: " + str(len(result)))

        if args.checkSign:
            with zilliqa.send_async_get_public_key(args.index, True):
                print("Waiting for user confirmation")
            rapdu = zilliqa.get_async_response()
            public_key, address = zilliqa.parse_get_public_key_response(rapdu.data)

            zilliqa.verify_signature(transaction, result, public_key)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    #parser.add_argument('--txnJson', '-j', type=str, required=False)
    parser.add_argument('--index', '-i', type=int, required=True)
    parser.add_argument('--checkSign', '-c', action='store_true', required=False)
    args = parser.parse_args()
    main(args)
