#/bin/bash

mkdir -p release
pushd release &> /dev/null

#for toolchain in armv7h-linux;
#for toolchain in multiarch-darwin;
for toolchain in i686-linux i686-w64-mingw32 x86_64-linux x86_64-w64-mingw32;
do
	echo $toolchain
	sudo rm -rf $toolchain
	mkdir $toolchain
	pushd $toolchain &> /dev/null
	cmake \
		-DCMAKE_TOOLCHAIN_FILE=../../cmake/$toolchain.cmake \
		-DCMAKE_OSX_ARCHITECTURES="x86_64;i386" \
		-DCMAKE_BUILD_TYPE=Release \
		-DBUILD_TESTING=1 \
		-DBUILD_COMMON_UI=0 \
		-DBUILD_SIMPLE_UI=0 \
		-DBUILD_WEB_UI=1 \
		-DUSE_FS_EVENT=1 \
		-DCPACK_SYSTEM_NAME="$toolchain" \
		-DPLUGIN_DEST=moony.lv2 \
		../..
	make -j4
	ARGS='-VV' make test
	sudo make package
	popd &> /dev/null
done

cp */*.zip .
popd &> /dev/null
