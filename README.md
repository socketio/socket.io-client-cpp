# SIO Client
This SIO client is depends on [websocket++](https://github.com/zaphoyd/websocketpp) and [rapidjson](https://github.com/miloyip/rapidjson), it provides another C++ client implementation for [Socket.IO](https://github.com/Automattic/socket.io).
This library is able to connect to a Socket.IO server 1.0, and the initial thought of writing it is inspired by the [socket.io-clientpp](https://github.com/ebshimizu/socket.io-clientpp) project.

C++ has amazing abilities to cross platform, there's a screen shot about using all example apps (iPhone,QT,Console and web) in this project to chat in one room.

![Clients with iPhone, QT, Console and web](/screenshots/QuadClients.png)

## Socket.IO 1.0+ protocol has been implemented!
The code is compatible with 1.0+ protocol only, not with prior protocols.

## C++11 only for now
C++11 saves much time for me, so this is C++11 only for the first version.
I'll do further compatibility efforts on demand.

## Supported features
1. Internal thread manangement.
2. Sends plain text messages.
3. Sends binary messages.
4. Sends structured messages with text and binary all together.
5. Sends messages with an ack and its corresponding callback.
6. Receives messages and automatically sends customable ack if need.
7. Automatically ping/pong messages and timeout management.
8. Reconnection.

## Usage
1. Make sure you have the boost libararies installed.
2. Include websocket++, rapidjson and `sio_client.cpp`,`sio_packet.cpp` in your project.
3. Include `sio_client.h` where you want to use it.
4. Use `message` and its derived classes to compose complex text/binary messages.

## Boost build instructions(Build the necessary subset only)
1. Download boost from [boost.org](http://www.boost.org/).(suppose we downloaded boost 1.55.0)
2. Unpack boost to some place.(such as D:\boost_1_55_0)
3. Run either .\bootstrap.bat (on Windows), or ./bootstrap.sh (on other operating systems) under boost folder(D:\boost_1_55_0).
4. Run ./b2 install --prefix=PREFIX
where PREFIX is a directory where you want Boost.Build to be installed.
5. Optionally, add PREFIX/bin to your PATH environment variable.
6. Build needed boost modules, with following command line as an example:

For Windows:
```shell
bjam stage --toolset=msvc --with-system --with-date_time --with-thread --with-regex --with-serialization --with-random --stagedir="D:\boost_1_55_0\boost_build\release" link=static runtime-link=shared threading=multi release
```
For iOS
```shell
bjam -j16 --with-system --with-date_time --with-thread --with-regex --with-serialization --with-random --build-dir=iphone-build --stagedir=iphone-build/stage --prefix=$PREFIXDIR toolset=darwin architecture=arm target-os=iphone macosx-version=iphone-`${IPHONE_SDKVERSION}` define=_LITTLE_ENDIAN link=static stage
```
For Mac OSX
```shell
b2 -j16 --with-system --with-date_time --with-thread --with-regex --with-serialization --with-random --build-dir=osx-build --stagedir=osx-build/stage toolset=clang cxxflags="-std=c++11 -stdlib=libc++ -arch i386 -arch x86_64" linkflags="-stdlib=libc++" link=static threading=multi stage
```
Finally, Add boost source folder to `header search path`, and add static libs to link option.

sio client specific source is released under the BSD license.
