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
4. Include `sio_client.h` in your client code where you want to use it.
