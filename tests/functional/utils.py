from pathlib import Path

from ragger.navigator import NavInsID, NavIns

ROOT_SCREENSHOT_PATH = Path(__file__).parent.resolve()


def get_nano_review_instructions(num_screen_skip):
    instructions = [NavIns(NavInsID.RIGHT_CLICK)] * num_screen_skip
    instructions.append(NavIns(NavInsID.BOTH_CLICK))
    return instructions


def get_fat_review_instructions(num_screen_skip):
    instructions = [NavIns(NavInsID.USE_CASE_REVIEW_TAP)] * num_screen_skip
    instructions.append(NavIns(NavInsID.USE_CASE_REVIEW_CONFIRM))
    instructions.append(NavIns(NavInsID.USE_CASE_STATUS_WAIT))
    return instructions
