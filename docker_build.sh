#!/bin/bash

BASE_NAME="moony.lv2"
CI_BUILD_NAME=$1
CI_BUILD_DIR="build-${CI_BUILD_NAME}"

PKG_CONFIG_PATH="/opt/lv2/lib/pkgconfig:/opt/${CI_BUILD_NAME}/lib/pkgconfig:/usr/lib/${CI_BUILD_NAME}/pkgconfig"

rm -rf ${CI_BUILD_DIR}
PKG_CONFIG_PATH=${PKG_CONFIG_PATH} meson \
	--prefix="/opt/${CI_BUILD_NAME}" \
	--libdir="lib" \
	--cross-file ${CI_BUILD_NAME} ${CI_BUILD_DIR} \
	-Db_lto=false -Db_lundef=true -Db_asneeded=true

sed -i \
	-e '/framework/s/-Wl,-O1//g' \
	-e '/framework/s/-Wl,--start-group//g' \
	-e '/framework/s/-Wl,--end-group//g' \
  -e '/framework/s/-Wl,-soname,.*dylib//g' \
  -e '/framework/s/-Wl,-rpath-link,[^ ]*//g' \
	${CI_BUILD_DIR}/build.ninja

ninja -C ${CI_BUILD_DIR}
