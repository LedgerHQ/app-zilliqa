#!/usr/bin/python3

import sys
import decimal

# https://stackoverflow.com/a/44702621/2128804
def floatToString(inputValue, shift):
    formatter = '{0:.' + shift + 'f}'
    return formatter.format(inputValue).rstrip('0').rstrip('.')


if len(sys.argv) == 2:
    qa = decimal.Decimal(sys.argv[1])
    shift = str(12)
else:
    print('Usage: verifier.py Qa (length of Qa < 30 digits)')
    sys.exit(1)

shift_0s = pow(10, int(shift))
zil = (qa / decimal.Decimal(shift_0s))
print(floatToString(zil, shift))
