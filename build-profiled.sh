#!/bin/bash
if [ "$#" != 1 ]
then
    echo "Usage: ./build-profiled.sh [image]" >&2
    echo "Uses the image to retrieve profiling information in order to build an" >&2
    echo "optimized xgbcdyna." >&2
    exit 1
fi

make clean
echo "Building profiling version..."
PROF=--coverage make || exit 1
./xgbcdyna $1
make clean
echo "Building optimized version..."
PROF=-fprofile-use make || exit 1
echo "Removing profiling information..."
rm -f *.gc??
