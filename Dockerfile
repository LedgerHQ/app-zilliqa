FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN rm /bin/sh && ln -s /bin/bash /bin/sh
RUN apt-get update -y  && \
	apt-get remove -y clang-10 gcc-9 && \
	apt-get autoremove && \
	apt-get install -y clang-9 gcc-10 g++-10 cpp-10 make curl && \
	apt-get install -y git libudev-dev libusb-1.0-0-dev python3-dev python3-pip python3-setuptools python3-venv python3-wheel  gcc-multilib g++-multilib pkg-config autoconf libtool libsecp256k1-dev && \
	apt-get install -y qemu-user-static libgmp3-dev && \
	apt-get install gcc libpq-dev wget -y && \
	apt-get clean && \
	rm -rf /var/lib/apt/lists/*

WORKDIR /ledger-app
ENV LEDGER_DIR=/ledger-app

ADD tests/functional/requirements.txt requirements.txt
RUN pip install --extra-index-url https://test.pypi.org/simple/ -r requirements.txt	&& \
	pip install ledgerblue && \
	pip install wheel pillow && \
	SECP_BUNDLED_EXPERIMENTAL=1 pip --no-cache-dir install --no-binary secp256k1 secp256k1 

RUN git clone https://github.com/LedgerHQ/nanos-secure-sdk && \
	pushd nanos-secure-sdk && git checkout e941fcd6c421e35efb2237d1ea033913cca8f2d1 && popd
	
RUN wget -O gcc.tar.bz2 "https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2?rev=78196d3461ba4c9089a67b5f33edf82a&hash=5631ACEF1F8F237389F14B41566964EC" && \
	mkdir devenv; tar -xvjf gcc.tar.bz2 --directory devenv

RUN ln -s /usr/bin/python3 /usr/bin/python && \
	ln -s /usr/bin/clang-9 /usr/bin/clang

ENV BOLOS_SDK=/ledger-app/nanos-secure-sdk/
ENV GCCPATH=/ledger-app/devenv/gcc-arm-none-eabi-10.3-2021.10/bin/	
ENV PATH=/ledger-app/devenv/gcc-arm-none-eabi-10.3-2021.10/bin/:$PATH


WORKDIR /ledger-app/app

ENTRYPOINT /bin/bash