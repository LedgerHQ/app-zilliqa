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
    with LedgerCommBackend(None, interface="hid") as backend:
        zilliqa = ZilliqaClient(backend)

        with zilliqa.send_async_get_public_key(args.index, args.dispAddr):
            print("Please accept the request on the device")
        rapdu = zilliqa.get_async_response()
        public_key, address = zilliqa.parse_get_public_key_response(rapdu.data)
        print("Address:", address)
        print("length: ", len(address))
        print("Public Key:", public_key.hex())
        print("length: ", len(public_key.hex()))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--index', '-i', type=int, required=True)
    parser.add_argument('--dispAddr', '-a', action='store_true', required=False)
    args = parser.parse_args()
    main(args)
