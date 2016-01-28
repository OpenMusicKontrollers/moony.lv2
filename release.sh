#/bin/bash

mkdir -p release
pushd release &> /dev/null

#for toolchain in armv7-linux i686-linux i686-w64-mingw32 x86_64-linux x86_64-w64-mingw32;
for toolchain in i686-linux i686-w64-mingw32 x86_64-linux x86_64-w64-mingw32;
do
	echo $toolchain
	sudo rm -rf $toolchain
	mkdir $toolchain
	pushd $toolchain &> /dev/null
	cmake \
		-DCMAKE_TOOLCHAIN_FILE=../../cmake/$toolchain.cmake \
		-DBUILD_COMMON_UI=0 \
		-DBUILD_SIMPLE_UI=1 \
		-DUSE_FS_EVENT=1 \
		-DBUILD_TESTING=0 \
		-DCPACK_SYSTEM_NAME="$toolchain" \
		../..
	make
	sudo make package
	popd &> /dev/null
done

cp */*.zip .
popd &> /dev/null
