#!/bin/bash
mkdir -p `pwd`/build
cd `pwd`/build/ && cmake .. && make
cd ..
