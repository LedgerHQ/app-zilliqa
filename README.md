# Zilliqa Ledger App for Nano S and Nano X

Zilliqa wallet application for Nano S and Nano X.

## Development Environment

We use the developement environment provided by Ledger through the docker image
`ledger-app-builder-lite:latest`. To start developing, run:

```sh
docker run -it -v $PWD:/app --user "$(id -u)":"$(id -g)" -v /dev/bus/usb:/dev/bus/usb  ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder-lite:latest
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

Note that in order to load, you may need to run the Docker image as privileged:

```sh
docker run -it -v $PWD:/app --privileged -v /dev/bus/usb:/dev/bus/usb  ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder-lite:latest
```

To uninstall:

```sh
make delete
```

Static analysis:

```sh
 make clean && scan-build --use-cc=clang -analyze-headers -enable-checker security -enable-checker unix -enable-checker valist -o scan-build --status-bugs make default
```
