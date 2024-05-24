# C function to convert Qa to Zil

## Build
Just running `make qatozil` inside this directory should build the executable `qatozil`.

## Running
`./qatozil 10000000000000`

## Testing
The testsuite can be run as `./test.sh` once the `qatozil` executable is built.
Or you can use simply run `make`.
It compares a set of Qa values (you can add more by editing the script) with it's Zil value obtained using a python script `verifier.py`.
