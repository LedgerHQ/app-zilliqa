from ragger.backend import SpeculosBackend
from ragger.backend.interface import RaisePolicy
from ragger.bip import calculate_public_key_and_chaincode, CurveChoice
from ragger.navigator import NavInsID, NavIns

from apps.zilliqa import ZilliqaClient, ErrorType
from utils import ROOT_SCREENSHOT_PATH

ZILLIQA_KEY_INDEX = 1


def check_get_public_key_resp(backend, key_index, public_key):
    if isinstance(backend, SpeculosBackend):
        path = "44'/313'/{}'/0'/0'".format(key_index)
        ref_public_key, _ = calculate_public_key_and_chaincode(CurveChoice.Secp256k1,
                                                               path,
                                                               compress_public_key=True)
        # Check against nominal Speculos seed expected results
        assert public_key.hex() == ref_public_key


def test_get_public_key_show_addr_accepted(firmware, backend, navigator, test_name):
    client = ZilliqaClient(backend)
    with client.send_async_get_public_key(ZILLIQA_KEY_INDEX, True):
        if firmware.device.startswith("nano"):
            navigator.navigate_until_text_and_compare(NavIns(NavInsID.RIGHT_CLICK),
                                                      [NavIns(NavInsID.BOTH_CLICK)],
                                                      "Approve",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        else:
            instructions = [
                NavIns(NavInsID.USE_CASE_REVIEW_TAP),
                NavIns(NavInsID.TOUCH, (200, 335)),
                NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_EXIT_QR),
                NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CONFIRM),
                NavIns(NavInsID.USE_CASE_STATUS_WAIT)
            ]
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                           test_name,
                                           instructions)
    response = client.get_async_response().data
    public_key, address = client.parse_get_public_key_response(response)
    check_get_public_key_resp(backend, ZILLIQA_KEY_INDEX, public_key)


def test_get_public_key_show_key_accepted(firmware, backend, navigator, test_name):
    client = ZilliqaClient(backend)
    with client.send_async_get_public_key(ZILLIQA_KEY_INDEX, False):
        if firmware.device.startswith("nano"):
            navigator.navigate_until_text_and_compare(NavIns(NavInsID.RIGHT_CLICK),
                                                      [NavIns(NavInsID.BOTH_CLICK)],
                                                      "Approve",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        else:
            instructions = [
                NavIns(NavInsID.USE_CASE_REVIEW_TAP),
                NavIns(NavInsID.TOUCH, (200, 335)),
                NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_EXIT_QR),
                NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CONFIRM),
                NavIns(NavInsID.USE_CASE_STATUS_WAIT)
            ]
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                           test_name,
                                           instructions)
    response = client.get_async_response().data
    public_key, address = client.parse_get_public_key_response(response)
    check_get_public_key_resp(backend, ZILLIQA_KEY_INDEX, public_key)


def test_get_public_key_show_addr_refused(firmware, backend, navigator, test_name):
    client = ZilliqaClient(backend)
    if firmware.device.startswith("nano"):
        with client.send_async_get_public_key(ZILLIQA_KEY_INDEX, True):
            backend.raise_policy = RaisePolicy.RAISE_NOTHING
            navigator.navigate_until_text_and_compare(NavIns(NavInsID.RIGHT_CLICK),
                                                      [NavIns(NavInsID.BOTH_CLICK)],
                                                      "Reject",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        rapdu = client.get_async_response()
        assert rapdu.status == ErrorType.SW_USER_REJECTED
        assert len(rapdu.data) == 0
    else:
        instructions_set = [
            [
                NavIns(NavInsID.USE_CASE_REVIEW_REJECT),
                NavIns(NavInsID.USE_CASE_STATUS_WAIT)
            ],
            [
                NavIns(NavInsID.USE_CASE_REVIEW_TAP),
                NavIns(NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CANCEL),
                NavIns(NavInsID.USE_CASE_STATUS_WAIT)
            ]
        ]
        for i, instructions in enumerate(instructions_set):
            with client.send_async_get_public_key(ZILLIQA_KEY_INDEX, True):
                backend.raise_policy = RaisePolicy.RAISE_NOTHING
                navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                               test_name + f"/part{i}",
                                               instructions)
            rapdu = client.get_async_response()
            assert rapdu.status == ErrorType.SW_USER_REJECTED
            assert len(rapdu.data) == 0
