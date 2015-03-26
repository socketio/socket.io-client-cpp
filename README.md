# SIO Client
This SIO client is depends on [websocket++](https://github.com/zaphoyd/websocketpp) and [rapidjson](https://github.com/miloyip/rapidjson), it provides another C++ client implementation for [Socket.IO](https://github.com/Automattic/socket.io).
This library is able to connect to a Socket.IO server 1.0, and the initial thought of writing it is inspired by the [socket.io-clientpp](https://github.com/ebshimizu/socket.io-clientpp) project.

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

sio client specific source is released under the BSD license.
