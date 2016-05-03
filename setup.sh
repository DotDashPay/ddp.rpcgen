#!/bin/bash

cwd=$(pwd)

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $DIR/thirdparty/uncrustify
./autogen.sh
./configure
make

cd $cwd
