# Socket.IO C++ Client

[![Build Status](https://github.com/socketio/socket.io-client-cpp/workflows/CI/badge.svg)](https://github.com/socketio/socket.io-client-cpp/actions)

By virtue of being written in C++, this client works in several different platforms. The [examples](https://github.com/socketio/socket.io-client-cpp/tree/master/examples) folder contains an iPhone, QT and Console example chat client! It depends on [websocket++](https://github.com/zaphoyd/websocketpp) and is inspired by [socket.io-clientpp](https://github.com/ebshimizu/socket.io-clientpp).

[![Clients with iPhone, QT, Console and web](https://cldup.com/ukvVVZmvYV.png)](https://github.com/socketio/socket.io-client-cpp/tree/master/examples)

## Compatibility table

<table>
  <tr>
    <th rowspan="2">C++ Client version</th>
    <th colspan="2">Socket.IO server version</th>
  </tr>
  <tr>
    <td align="center">1.x / 2.x</td>
    <td align="center">3.x / 4.x</td>
  </tr>
  <tr>
    <td>2.x (<code>2.x</code> branch)</td>
    <td align="center">YES</td>
    <td align="center">YES, with <code><a href="https://socket.io/docs/v4/server-initialization/#allowEIO3">allowEIO3: true</a></code></td>
  </tr>
  <tr>
    <td>3.x (<code>master</code> branch)</td>
    <td align="center">NO</td>
    <td align="center">YES</td>
  </tr>
</table>

## Features

- 100% written in modern C++11
- Binary support
- Automatic JSON encoding
- Multiplex support
- Similar API to the Socket.IO JS client
- Cross platform

Note: Only the WebSocket transport is currently implemented (no fallback to HTTP long-polling)

## Installation alternatives

* [With CMAKE](./INSTALL.md#with-cmake)
* [Without CMAKE](./INSTALL.md#without-cmake)
* [iOS and OS X](./INSTALL_IOS.md)
 * Option 1: Cocoapods
 * Option 2: Create a static library
 * Option 3: Manual integration


## Quickstart

** [Full overview of API can be seen here](./API.md) **


The APIs are similar to the JS client.

#### Connect to a server
```C++
sio::client h;
h.connect("http://127.0.0.1:3000");
```

#### Emit an event

```C++
// emit event name only:
h.socket()->emit("login");

// emit text
h.socket()->emit("add user", username);

// emit binary
char buf[100];
h.socket()->emit("add user", std::make_shared<std::string>(buf,100));

// emit message object with lambda ack handler
h.socket()->emit("add user", string_message::create(username), [&](message::list const& msg) {
});

// emit multiple arguments
message::list li("sports");
li.push(string_message::create("economics"));
socket->emit("categories", li);
```
Items in `message::list` will be expanded in server side event callback function as function arguments.

#### Bind an event

##### Bind with function pointer
```C++
void OnMessage(sio::event &)
{

}
h.socket()->on("new message", &OnMessage);
```

##### Bind with lambda
```C++
h.socket()->on("login", [&](sio::event& ev)
{
    //handle login message
    //post to UI thread if any UI updating.
});
```

##### Bind with member function
```C++
class MessageHandler
{
public:
    void OnMessage(sio::event &);
};
MessageHandler mh;
h.socket()->on("new message",std::bind( &MessageHandler::OnMessage,&mh,std::placeholders::_1));
```

#### Using namespace
```C++
h.socket("/chat")->emit("add user", username);
```
** [Full overview of API can be seen here](./API.md) **

## License

MIT
