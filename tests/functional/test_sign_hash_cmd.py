from ragger.backend import SpeculosBackend
from ragger.backend.interface import RaisePolicy
from ragger.bip import calculate_public_key_and_chaincode, CurveChoice

from ragger.navigator import NavInsID

from apps.zilliqa import ZilliqaClient, ErrorType

from utils import ROOT_SCREENSHOT_PATH, get_nano_review_instructions
from utils import get_fat_review_instructions

ZILLIQA_KEY_INDEX = 1


def check_signature(client, backend, message, response):
    if isinstance(backend, SpeculosBackend):
        path = "44'/313'/{}'/0'/0'".format(ZILLIQA_KEY_INDEX)
        ref_public_key, _ = calculate_public_key_and_chaincode(CurveChoice.Secp256k1,
                                                               path,
                                                               compress_public_key=True)
        public_key = bytes.fromhex(ref_public_key)
    else:
        response = client.send_get_public_key_non_confirm(ZILLIQA_KEY_INDEX)
        public_key, address = client.parse_get_public_key_response(response.data)
        
    client.verify_signature(message, response, public_key)


def test_sign_hash_accepted(test_name, firmware, backend, navigator):
    message = "02E681C8EB3602CDB9261F407E2C2EE6CB9BA996AAA895677E133C02BEFC1F84"
    message_bytes = bytes.fromhex(message)

    client = ZilliqaClient(backend)
    # Can't use navigate_until_text_and_compare because of the first screen and
    # approve screen both displaying "Sign" text.
    if firmware.device == "nanos":
        instructions = get_nano_review_instructions(5)
    elif firmware.device.startswith("nano"):
        instructions = get_nano_review_instructions(3)
    else:
        instructions = get_fat_review_instructions(2)
    with client.send_async_sign_hash_message(ZILLIQA_KEY_INDEX, message_bytes):
        navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                       test_name,
                                       instructions)
    response = client.get_async_response().data
    check_signature(client, backend, message_bytes, response)


def test_sign_hash_refused(test_name, firmware, backend, navigator):
    message = "02E681C8EB3602CDB9261F407E2C2EE6CB9BA996AAA895677E133C02BEFC1F84"
    message_bytes = bytes.fromhex(message)

    client = ZilliqaClient(backend)
    if firmware.device.startswith("nano"):
        with client.send_async_sign_hash_message(ZILLIQA_KEY_INDEX, message_bytes):
            backend.raise_policy = RaisePolicy.RAISE_NOTHING
            navigator.navigate_until_text_and_compare(NavInsID.RIGHT_CLICK,
                                                      [NavInsID.BOTH_CLICK],
                                                      "Cancel",
                                                      ROOT_SCREENSHOT_PATH,
                                                      test_name)
        rapdu = client.get_async_response()
        assert rapdu.status == ErrorType.SW_USER_REJECTED
        assert len(rapdu.data) == 0
    else:
        instructions_set = []
        for i in range(3):
            instructions_set.append([NavInsID.USE_CASE_REVIEW_TAP] * i +
                                    [NavInsID.USE_CASE_REVIEW_REJECT] +
                                    [NavInsID.USE_CASE_CHOICE_CONFIRM] +
                                    [NavInsID.USE_CASE_STATUS_DISMISS])
        for i, instructions in enumerate(instructions_set):
            with client.send_async_sign_hash_message(ZILLIQA_KEY_INDEX, message_bytes):
                backend.raise_policy = RaisePolicy.RAISE_NOTHING
                navigator.navigate_and_compare(ROOT_SCREENSHOT_PATH,
                                               test_name + f"/part{i}",
                                               instructions)
            rapdu = client.get_async_response()
            assert rapdu.status == ErrorType.SW_USER_REJECTED
            assert len(rapdu.data) == 0
