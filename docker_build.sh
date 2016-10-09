#!/bin/bash

TARGET=$1

CI_PROJECT_DIR=$(pwd)
CI_BUILD_NAME=${TARGET}

BASE_NAME="moony.lv2"
PKG_CONFIG_PATH="/opt/lv2/lib/pkgconfig:/opt/${CI_BUILD_NAME}/lib/pkgconfig"
TOOLCHAIN_FILE="${CI_PROJECT_DIR}/cmake/${CI_BUILD_NAME}.cmake"

apt-get install -y -q xsltproc

#git clone https://gitlab.com/OpenMusicKontrollers/moony.lv2.git

mkdir -p ${TARGET}
pushd ${TARGET}
	PKG_CONFIG_PATH=${PKG_CONFIG_PATH} cmake \
		-DCMAKE_BUILD_TYPE=Release \
		-DUSE_VERBOSE_LOG=0 \
		-DBUILD_INLINE_DISPLAY=1 \
		-DBUILD_TESTING=1 \
		-DCMAKE_INSTALL_PREFIX=${CI_PROJECT_DIR} \
		-DPLUGIN_DEST="${BASE_NAME}-$(cat ../VERSION)/${CI_BUILD_NAME}/${BASE_NAME}" \
		-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_FILE} \
		..
	make -j4
	make install
	#ARGS='-VV' make test
popd
