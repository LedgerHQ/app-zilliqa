#!/bin/sh

rm -rf tests/elfs
mkdir -p tests/elfs

make clean
make BOLOS_SDK=${NANOS_SDK}
cp bin/app.elf tests/elfs/zilliqa_nanos.elf

make clean
make BOLOS_SDK=${NANOSP_SDK}
cp bin/app.elf tests/elfs/zilliqa_nanosp.elf

make clean
make BOLOS_SDK=${NANOX_SDK}
cp bin/app.elf tests/elfs/zilliqa_nanox.elf

make clean