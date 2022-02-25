## Install

### With CMake
1. Use `git clone --recurse-submodules https://github.com/socketio/socket.io-client-cpp.git` to clone your local repo.
2. Run `cmake  ./`
3. Run `make install`(if makefile generated) or open generated project (if project file generated) to build.
4. Outputs is under `./build`, link with the all static libs under `./build/lib` and  include headers under `./build/include` in your client code where you want to use it.

### Without CMake
1. Use `git clone --recurse-submodules https://github.com/socketio/socket.io-client-cpp.git` to clone your local repo.
2. Add `./lib/asio/asio/include`, `./lib/websocketpp` and `./lib/rapidjson/include` to headers search path.
3. Include all files under `./src` in your project, add `sio_client.cpp`,`sio_socket.cpp`,`internal/sio_client_impl.cpp`, `internal/sio_packet.cpp` to source list.
4. Add `BOOST_DATE_TIME_NO_LIB`, `BOOST_REGEX_NO_LIB`, `ASIO_STANDALONE`, `_WEBSOCKETPP_CPP11_STL_` and `_WEBSOCKETPP_CPP11_FUNCTIONAL_` to the preprocessor definitions
5. Include `sio_client.h` in your client code where you want to use it.

### With vcpkg

You can download and install the Socket.IO C++ client using the [vcpkg](https://github.com/Microsoft/vcpkg) dependency manager:

```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh
./vcpkg integrate install
vcpkg install socket-io-client
```

The Socket.IO client port in vcpkg is kept up to date by Microsoft team members and community contributors. If the version is out of date, please [create an issue or pull request](https://github.com/Microsoft/vcpkg) on the vcpkg repository.
