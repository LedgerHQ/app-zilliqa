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
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Approve",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        else:
            instructions = [
                NavInsID.USE_CASE_REVIEW_TAP,
                NavIns(NavInsID.TOUCH, (200, 335)),
                NavInsID.USE_CASE_ADDRESS_CONFIRMATION_EXIT_QR,
                NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CONFIRM,
                NavInsID.USE_CASE_STATUS_DISMISS
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
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Approve",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        else:
            instructions = [
                NavInsID.USE_CASE_REVIEW_TAP,
                NavIns(NavInsID.TOUCH, (200, 335)),
                NavInsID.USE_CASE_ADDRESS_CONFIRMATION_EXIT_QR,
                NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CONFIRM,
                NavInsID.USE_CASE_STATUS_DISMISS
            ]
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                           test_name,
                                           instructions)
    response = client.get_async_response().data
    public_key, address = client.parse_get_public_key_response(response)
    check_get_public_key_resp(backend, ZILLIQA_KEY_INDEX, public_key)


def test_get_public_key_silent(backend, navigator, test_name):
    client = ZilliqaClient(backend)
    response = client.send_get_public_key_non_confirm(ZILLIQA_KEY_INDEX)
    public_key, address = client.parse_get_public_key_response(response.data)
    check_get_public_key_resp(backend, ZILLIQA_KEY_INDEX, public_key)


def test_get_public_key_show_addr_refused(firmware, backend, navigator, test_name):
    client = ZilliqaClient(backend)
    if firmware.device.startswith("nano"):
        with client.send_async_get_public_key(ZILLIQA_KEY_INDEX, True):
            backend.raise_policy = RaisePolicy.RAISE_NOTHING
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Reject",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        rapdu = client.get_async_response()
        assert rapdu.status == ErrorType.SW_USER_REJECTED
        assert len(rapdu.data) == 0
    else:
        instructions_set = [
            [
                NavInsID.USE_CASE_REVIEW_REJECT,
                NavInsID.USE_CASE_STATUS_DISMISS
            ],
            [
                NavInsID.USE_CASE_REVIEW_TAP,
                NavInsID.USE_CASE_ADDRESS_CONFIRMATION_CANCEL,
                NavInsID.USE_CASE_STATUS_DISMISS
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

        # Test with public key display
        i += 1
        with client.send_async_get_public_key(ZILLIQA_KEY_INDEX, False):
            backend.raise_policy = RaisePolicy.RAISE_NOTHING
            navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                           test_name + f"/part{i}",
                                           instructions)
        rapdu = client.get_async_response()
        assert rapdu.status == ErrorType.SW_USER_REJECTED
        assert len(rapdu.data) == 0
