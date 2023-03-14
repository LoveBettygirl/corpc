#!/bin/bash
cd `pwd`/build/ && make uninstall
cd ..
ldconfig /usr/local/lib
