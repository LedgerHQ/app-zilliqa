#!/bin/sh

pytest tests/functional/ -v --device nanos
pytest tests/functional/ -v --device nanosp
pytest tests/functional/ -v --device nanox