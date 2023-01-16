from pathlib import Path

from ragger.navigator import NavInsID, NavIns

ROOT_SCREENSHOT_PATH = Path(__file__).parent.resolve()


def get_nano_review_instructions(num_screen_skip):
    instructions = [NavIns(NavInsID.RIGHT_CLICK)] * num_screen_skip
    instructions.append(NavIns(NavInsID.BOTH_CLICK))
    return instructions
