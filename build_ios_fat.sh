#!/bin/bash

# git clone --recurse-submodules https://github.com/socketio/socket.io-client-cpp.git
# git clone https://github.com/leetal/ios-cmake
# cd socket.io-client-cpp

export DARWIN_DEPLOYMENT_TARGET=8.0

archs="armv7 armv7s arm64 i386 x86_64"

rm -rf build-ios-*/
rm libssl.a
rm libcrypto.a
rm libboost_*.a
rm libsioclient_tls.a

for arch in $archs; do
	mkdir -p build-ios-$arch && pushd build-ios-$arch

	conan install .. --profile ~/.conan/profiles/ios-$arch --build

	platform="OS"
	if [ "$arch" = "i386" ]; then
		platform="SIMULATOR"
	elif [ "$arch" = "x86_64" ]; then
		platform="SIMULATOR64"
	fi

	cmake -DCMAKE_TOOLCHAIN_FILE=../ios-cmake/ios.toolchain.cmake -DIOS_PLATFORM=$platform ..
	cmake --build .

	popd
done

lipo -create build-ios-arm64/libsioclient_tls.a build-ios-x86_64/libsioclient_tls.a build-ios-i386/libsioclient_tls.a -output libsioclient_tls.a

lipo -create ~/.conan/data/OpenSSL/1.1.1/conan/stable/package/*/lib/libssl.a -output libssl.a
lipo -create ~/.conan/data/OpenSSL/1.1.1/conan/stable/package/*/lib/libcrypto.a -output libcrypto.a

lipo -create ~/.conan/data/boost/1.67.0/conan/stable/package/*/lib/libboost_contract.a -output libboost_contract.a
lipo -create ~/.conan/data/boost/1.67.0/conan/stable/package/*/lib/libboost_system.a -output libboost_system.a
lipo -create ~/.conan/data/boost/1.67.0/conan/stable/package/*/lib/libboost_random.a -output libboost_random.a
lipo -create ~/.conan/data/boost/1.67.0/conan/stable/package/*/lib/libboost_date_time.a -output libboost_date_time.a
