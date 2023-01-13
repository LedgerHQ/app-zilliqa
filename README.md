# Zilliqa Ledger App for Nano S and Nano X

Zilliqa wallet application for Nano S and Nano X.

## Development Environment

To build environment:

```sh
docker build .  --tag builder_image
```

To start developing:

```sh
docker run -it -v $PWD:/ledger-app/app --privileged -v /dev/bus/usb:/dev/bus/usb builder_image:latest bash
```

## Inside Dev Environment

To build app:

```sh
make clean && make
```

To load the app:

```sh
make load
```

To uninstall:

```sh
make delete
```
