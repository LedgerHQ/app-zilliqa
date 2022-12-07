#!/usr/bin/env python3

import sys
import argparse

from pathlib import Path

from ragger.backend import LedgerCommBackend

REPO_ROOT_DIRECTORY = Path(__file__).parent
ZILLIQA_LIB_DIRECTORY = (REPO_ROOT_DIRECTORY / "../tests/functional/apps").resolve().as_posix()
sys.path.append(ZILLIQA_LIB_DIRECTORY)
from zilliqa import ZilliqaClient


def main(args):
    mhash = args.mhash
    if len(mhash) > 64:
        mhash = mhash[:64]
    hashBytes = bytes.fromhex(mhash)

    with LedgerCommBackend(None, interface="hid") as backend:
        zilliqa = ZilliqaClient(backend)

        with zilliqa.send_async_sign_hash_message(args.index, hashBytes):
            print("Please accept the request on the device")

        result = zilliqa.get_async_response().data
        print("Response: " + result.hex())
        print("Length: " + str(len(result)))

        if args.checkSign:
            with zilliqa.send_async_get_public_key(args.index, True):
                print("Waiting for user confirmation")
            rapdu = zilliqa.get_async_response()
            public_key, address = zilliqa.parse_get_public_key_response(rapdu.data)

            zilliqa.verify_signature(hashBytes, result, public_key)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--mhash', '-m', type=str, required=True)
    parser.add_argument('--index', '-i', type=int, required=True)
    parser.add_argument('--checkSign', '-c', action='store_true', required=False)
    args = parser.parse_args()
    main(args)
