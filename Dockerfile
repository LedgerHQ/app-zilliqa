FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive
RUN rm /bin/sh && ln -s /bin/bash /bin/sh
RUN apt-get update -y  && \
	apt-get remove -y clang-10 && \
	apt-get autoremove && \
	apt-get install -y clang-9 make && \
	apt-get install -y git libudev-dev libusb-1.0-0-dev python3-dev python3-pip python3-setuptools python3-venv python3-wheel  gcc-multilib g++-multilib clang pkg-config autoconf libtool libsecp256k1-dev && \
	apt-get install gcc libpq-dev wget -y && \
	apt-get clean && \
	rm -rf /var/lib/apt/lists/*



WORKDIR /ledger-app
ENV LEDGER_DIR=/ledger-app

RUN pip install ledgerblue && \
	pip install wheel pillow && \
	SECP_BUNDLED_EXPERIMENTAL=1 pip --no-cache-dir install --no-binary secp256k1 secp256k1

RUN git clone https://github.com/LedgerHQ/nanos-secure-sdk && \
	pushd nanos-secure-sdk && git checkout nanos-1612 && popd && \
	wget https://launchpad.net/gcc-arm-embedded/5.0/5-2016-q1-update/+download/gcc-arm-none-eabi-5_3-2016q1-20160330-linux.tar.bz2 && \
	mkdir devenv; tar -xvjf gcc-arm-none-eabi-5_3-2016q1-20160330-linux.tar.bz2 --directory devenv

ENV BOLOS_SDK=/ledger-app/nanos-secure-sdk/
ENV GCCPATH=/ledger-app/devenv/gcc-arm-none-eabi-5_3-2016q1/bin/	
ENV PATH=/ledger-app/devenv/gcc-arm-none-eabi-5_3-2016q1/bin/:$PATH


WORKDIR /ledger-app/app

ENTRYPOINT /bin/bash