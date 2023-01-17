# Zilliqa Ledger App for Nano S and Nano X

Zilliqa wallet application for Nano S and Nano X.

## Build environment

To build all elfs, run:

```sh
docker run -it -v $PWD:/app --user "$(id -u)":"$(id -g)" ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder-lite:latest ./tools/build-all.sh
```

To build test environment, run:

```sh
docker build .  --tag builder_image
```

Then run the tests:

```sh
docker run -it -v $PWD:/app builder_image:latest ./tools/test-all.sh
```

## Build and load image

To build, start the Docker image with:

```sh
docker run -it -v $PWD:/app --user "$(id -u)":"$(id -g)" ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder-lite:latest
```

and run

```sh
make BOLOS_SDK=[SDK]
```

where `[SDK]` is `${NANOS_SDK}`, `${NANOSP_SDK}` or `${NANOX_SDK}`. Then exit
the docker image. Next load the

```sh
docker run -it -v $PWD:/app --user "$(id -u)":"$(id -g)" --privileged builder_image:latest
```

To load the app:

```sh
make load
```

To uninstall:

```sh
make delete
```

To run tests:

```sh
PROTOCOL_BUFFERS_PYTHON_IMPLEMENTATION=python pytest tests/functional/ -v --device [device]
```
