#!/bin/bash
set -e
rm -r release || true
rm arduino-husarnet-esp32-v1.zip || true
mkdir -p release/tools

(cd release/tools; mkdir -p esptool; cp ../../tools/esptool.py esptool/)
(cd release/tools/esptool; wget https://cdn.atomshare.net/166eb4f305fbcedb0040dc79df20a0443269ff00/esptool.exe)

names="$(git ls-files) $(find ./libraries/BLE/*)"
for name in $names; do
    mkdir -p release/$(dirname $name)
    if [ -d $name ]; then continue; fi
    cp $name release/$name
done
zip -r arduino-husarnet-esp32-v1.zip release
