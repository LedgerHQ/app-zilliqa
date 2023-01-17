FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
RUN rm /bin/sh && ln -s /bin/bash /bin/sh
RUN apt-get update -y  && \
	apt-get install -y qemu-user-static libgmp3-dev libudev-dev libusb-1.0-0-dev python3-dev python3-pip python3-setuptools python3-venv python3-wheel && \
	rm -rf /var/lib/apt/lists/*

WORKDIR /app

ADD tests/functional/requirements.txt requirements.txt
RUN pip install --extra-index-url https://test.pypi.org/simple/ -r requirements.txt

RUN ln -s /usr/bin/python3 /usr/bin/python

ENV PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=python
CMD /bin/bash