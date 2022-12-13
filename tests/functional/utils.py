from pathlib import Path

from ragger.navigator import NavInsID

ROOT_SCREENSHOT_PATH = Path(__file__).parent.resolve()


def get_nano_review_instructions(num_screen_skip):
    instructions = [NavInsID.RIGHT_CLICK] * num_screen_skip
    instructions.append(NavInsID.BOTH_CLICK)
    return instructions


def get_fat_review_instructions(num_screen_skip):
    instructions = [NavInsID.USE_CASE_REVIEW_TAP] * num_screen_skip
    instructions.append(NavInsID.USE_CASE_REVIEW_CONFIRM)
    instructions.append(NavInsID.USE_CASE_STATUS_DISMISS)
    return instructions
