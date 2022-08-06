#!/bin/bash

if [ ! -d "vcpkg" ]; then
    git clone --depth 1 https://github.com/microsoft/vcpkg
    ./vcpkg/bootstrap-vcpkg.sh
fi

./vcpkg/vcpkg install --overlay-triplets=custom-triplets --triplet=x64-osx-10.9 zlib libogg opus opusfile libvorbis
