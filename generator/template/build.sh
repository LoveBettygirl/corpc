#!/bin/bash

set -e
mkdir -p `pwd`/build
mkdir -p `pwd`/bin

cd `pwd`/build/ &&
    cmake .. &&
    make

cd ..