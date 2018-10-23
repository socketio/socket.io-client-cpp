#!/bin/bash

export DARWIN_DEPLOYMENT_TARGET=8.0

build_type=${1:-Release}
rebuild=${2}
archs="armv7 armv7s armv8 x86 x86_64"

rm -rf build-ios-*/
rm libssl.a
rm libcrypto.a
rm libboost_*.a
rm libsioclient_tls.a

if [ ! -d ios-cmake ]; then
	git clone https://github.com/leetal/ios-cmake
fi

for arch in $archs; do
	mkdir -p build-ios-$arch && pushd build-ios-$arch

	conan install .. -s arch=${arch} -s compiler=apple-clang -s compiler.version=9.1 -s os=iOS -s os.version=8.0 -s compiler.libcxx=libc++ -s build_type=${build_type} ${rebuild}

	platform="OS"
	if [ "$arch" = "x86" ]; then
		platform="SIMULATOR"
	elif [ "$arch" = "x86_64" ]; then
		platform="SIMULATOR64"
	fi

	cmake -DCMAKE_TOOLCHAIN_FILE=../ios-cmake/ios.toolchain.cmake -DIOS_PLATFORM=$platform -DIOS_DEPLOYMENT_TARGET=8.0 -DENABLE_BITCODE=YES -DCMAKE_BUILD_TYPE=${build_type} ..
	cmake --build .

	popd
done

lipo -create build-ios-armv8/libsioclient_tls.a build-ios-x86_64/libsioclient_tls.a build-ios-x86/libsioclient_tls.a -output libsioclient_tls.a

lipo -create ~/.conan/data/OpenSSL/1.1.1/conan/stable/package/*/lib/libssl.a -output libssl.a
lipo -create ~/.conan/data/OpenSSL/1.1.1/conan/stable/package/*/lib/libcrypto.a -output libcrypto.a

lipo -create ~/.conan/data/boost/1.67.0/conan/stable/package/*/lib/libboost_contract.a -output libboost_contract.a
lipo -create ~/.conan/data/boost/1.67.0/conan/stable/package/*/lib/libboost_system.a -output libboost_system.a
lipo -create ~/.conan/data/boost/1.67.0/conan/stable/package/*/lib/libboost_random.a -output libboost_random.a
lipo -create ~/.conan/data/boost/1.67.0/conan/stable/package/*/lib/libboost_date_time.a -output libboost_date_time.a
