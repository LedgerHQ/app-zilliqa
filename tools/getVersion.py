#!/usr/bin/env python3

import sys
import argparse

from pathlib import Path

from ragger.backend import LedgerCommBackend

REPO_ROOT_DIRECTORY = Path(__file__).parent
ZILLIQA_LIB_DIRECTORY = (REPO_ROOT_DIRECTORY / "../tests/functional/apps").resolve().as_posix()
sys.path.append(ZILLIQA_LIB_DIRECTORY)
from zilliqa import ZilliqaClient


def main():
    with LedgerCommBackend(None, interface="hid") as backend:
        zilliqa = ZilliqaClient(backend)
        version = zilliqa.send_get_version()
        print("v{}.{}.{}".format(version[0], version[1], version[2]))


if __name__ == "__main__":
    main()
