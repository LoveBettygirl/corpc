#!/bin/bash
cd `pwd`/build/ && make install
cd ..
ldconfig /usr/local/lib
