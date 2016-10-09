#!/usr/bin/bash

TARGET=$1

docker run --rm -it -v $(pwd):/workdir/moony.lv2 ventosus/${TARGET}
