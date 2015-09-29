## Install

### With CMake
1. Install boost, see [Boost setup](#boost_setup) section.
2. Use `git clone --recurse-submodules https://github.com/socketio/socket.io-client-cpp.git` to clone your local repo.
3. Run `cmake -DBOOST_ROOT:STRING=<your boost install folder> -DBOOST_VER:STRING=<your boost version> ./`
4. Run `make install`(if makefile generated) or open generated project (if project file generated) to build.
5. Outputs is under `./build`, link with the all static libs under `./build/lib` and  include headers under `./build/include` in your client code where you want to use it.

* If you're using boost without install,you can specify `boost include dir` and `boost lib dir` separately by:
```bash
cmake
-DBOOST_INCLUDEDIR=<your boost include folder>
-DBOOST_LIBRARYDIR=<your boost lib folder>
-DBOOST_VER:STRING=<your boost version>
./
```
* CMake didn't allow merging static libraries,but they're all copied to `./build/lib`, you can DIY if you like.

### Without CMake
1. Install boost, see [Boost setup](#boost_setup) section.
2. Use `git clone --recurse-submodules https://github.com/socketio/socket.io-client-cpp.git` to clone your local repo.
3. Add `<your boost install folder>/include`,`./lib/websocketpp` and `./lib/rapidjson/include` to headers search path.
4. Include all files under `./src` in your project, add `sio_client.cpp`,`sio_socket.cpp`,`internal/sio_client_impl.cpp`, `internal/sio_packet.cpp` to source list.
5. Add `<your boost install folder>/lib` to library search path, add `boost.lib`(Win32) or `-lboost`(Other) link option.
6. Include `sio_client.h` in your client code where you want to use it.

## Boost setup

1. Download boost from [boost.org](http://www.boost.org/).
1. Unpack boost to some place.
1. Run either .\bootstrap.bat (on Windows), or ./bootstrap.sh (on other operating systems) under boost folder.

## Boost build (Build the necessary subset only)
Windows (or other mainstream desktop platforms shall work too):

The following script will build the necessary subset:

```bash
bjam install --prefix="<your boost install folder>" --with-system --with-date_time --with-random link=static runtime-link=shared threading=multi
```
Optionally You can merge all output .lib files into a fat one, especially if you're not using cmake.

In output folder, run:

```bash
lib.exe /OUT:boost.lib *
```
