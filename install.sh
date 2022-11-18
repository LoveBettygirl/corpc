#!/bin/bash
cp -r `pwd`/lib/corpc /usr/local/include
cp `pwd`/lib/corpc.a /usr/local/lib
ldconfig /usr/local/lib