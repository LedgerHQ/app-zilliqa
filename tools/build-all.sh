#!/bin/sh

make clean
make BOLOS_SDK=${NANOS_SDK}

make clean
make BOLOS_SDK=${NANOSP_SDK}

make clean
make BOLOS_SDK=${NANOX_SDK}

make clean
make BOLOS_SDK=${STAX_SDK}

make clean