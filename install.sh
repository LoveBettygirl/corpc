#!/bin/bash
cp -r `pwd`/lib/corpc /usr/local/include
cp `pwd`/lib/libcorpc.a /usr/local/lib
ldconfig /usr/local/lib