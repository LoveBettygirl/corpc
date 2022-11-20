#!/bin/bash

src_root="corpc"

function copy_header() {
    for file in `ls $src_root$1`
    do
        filename=$1"/"$file
        src_file=$src_root$filename
        dst_file=`pwd`/lib/corpc$filename
        if [ -d $src_file ]
        then
            mkdir -p $dst_file
            copy_header $filename
        elif [ "${src_file##*.}"x = "h"x ]
        then
            cp $src_file $dst_file
        fi
    done
}

set -e
mkdir -p `pwd`/build
mkdir -p `pwd`/bin
mkdir -p `pwd`/lib
mkdir -p `pwd`/lib/corpc

cd `pwd`/build/ &&
    cmake .. &&
    make

# 回到项目根目录
cd ..

# 生成项目头文件
copy_header /