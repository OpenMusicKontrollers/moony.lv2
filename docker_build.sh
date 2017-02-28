#!/bin/bash

TARGET=$1

CI_PROJECT_DIR=$(pwd)
CI_BUILD_NAME=${TARGET}

BASE_NAME="moony.lv2"
PKG_CONFIG_PATH="/opt/lv2/lib/pkgconfig:/opt/${CI_BUILD_NAME}/lib/pkgconfig"
TOOLCHAIN_FILE="${CI_PROJECT_DIR}/cmake/${CI_BUILD_NAME}.cmake"

rm -rf ${TARGET}
mkdir -p ${TARGET}
pushd ${TARGET}
	PKG_CONFIG_PATH=${PKG_CONFIG_PATH} cmake \
    -DCAIRO_INCLUDE_DIRS="/opt/${CI_BUILD_NAME}/include" \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_INLINE_DISPLAY=1 \
		-DBUILD_TESTING=0 \
		-DCMAKE_INSTALL_PREFIX=${CI_PROJECT_DIR} \
		-DPLUGIN_DEST="${BASE_NAME}-$(cat ../VERSION)/${CI_BUILD_NAME}/${BASE_NAME}" \
		-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE} \
		..
	make -j4
	make install
	#ARGS='-VV' make test
popd
