#!/bin/sh

pushd device
make all
popd

./host/dialogtool.py -p /dev/ttyUSB0 chip_id
