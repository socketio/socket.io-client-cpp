# Socket.IO C++ Client

This repository contains the Socket.IO C++ client. It depends on [websocket++](https://github.com/zaphoyd/websocketpp) and is inspired by [socket.io-clientpp](https://github.com/ebshimizu/socket.io-clientpp).

By virtue of being written in C++, this client works in several different platforms. The [examples](https://github.com/socketio/socket.io-client-cpp/tree/master/examples) folder contains an iPhone, QT and Console example chat client!

[![Clients with iPhone, QT, Console and web](https://cldup.com/ukvVVZmvYV.png)](https://github.com/socketio/socket.io-client-cpp/tree/master/examples)

## Features

- 100% written in modern C++11
- Compatible with 1.0+ protocol
- Binary support
- Automatic JSON encoding
- Similar API to the Socket.IO JS client

## How to use

1. Install boost
2. Use `git clone --recurse-submodules https://github.com/socketio/socket.io-client-cpp.git` to clone your local repo.
3. Include `websocket++`, `rapidjson` and `sio_client.cpp`,`sio_packet.cpp` in your project.
4. Include `sio_client.h` where you want to use it.
5. Use `message` and its derived classes to compose complex text/binary messages.

## API

### Constructors

`client()` default constructor.

### Event Emitter
`void emit(std::string const& name, std::string const& message)`

`void emit(std::string const& name, std::string const& message, std::function<void (message::ptr const&)> const& ack)`

Emit a plain text message, along with event's name and a optional ack callback function if you need server ack.

`void emit(std::string const& name, message::ptr const& args)`

`void emit(std::string const& name, message::ptr const& args, std::function<void (message::ptr const&)> const& ack)`

Emit a `message` (explained below) object, along with event's name and a optional ack callback function if you need server ack.

`void emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr)`

`void emit(std::string const& name, std::shared_ptr<const std::string> const& binary_ptr, std::function<void (message::ptr const&)> const& ack)`

Emit a single binary buffer, along with event's name and a optional ack callback function if you need server ack.

### Event Bindings
`void bind_event(std::string const& event_name,event_listener const& func)`

Bind a callback to specified event name. Same as `socket.on` function in JS.

`void unbind_event(std::string const& event_name)`

Unbind the event callback with specified name.

`void clear_event_bindings()` 

Clear all event bindings.

`void set_default_event_listener(event_listener const& l)`

Set a default event handler for events with no binding functions.

`void set_error_listener(error_listener const& l)`

Set the error handler for socket.io error messages.

```C++
//event listener declare:
typedef std::function<void(const std::string& name,message::ptr const& message,bool need_ack, message::ptr& ack_message)> event_listener;
        
typedef std::function<void(message::ptr const& message)> error_listener;

```

### Connection Listeners
`void set_open_listener(con_listener const& l)`

Call when websocket is open, especially means good connectivity.

`void set_connect_listener(con_listener const& l)`

Call when socket.io `connect` message is received, ready to send socket.io messages.

`void set_fail_listener(con_listener const& l)`

Call when failed in connecting.

`void set_close_listener(close_listener const& l)`

Call when closed or drop. See `client::close_reason`

```C++
//connection listener declare:
enum close_reason
{
    close_reason_normal,
    close_reason_drop
};
typedef std::function<void(void)> con_listener;
        
typedef std::function<void(close_reason const& reason)> close_listener;
```
### Connect and Close
`void connect(const std::string& uri)`

Connect to socket.io server, eg. `client.connect("ws://localhost:3000");`
        
`void reconnect(const std::string& uri)`

Try to reconnect with original session id. If `fail listener` triggered, means your session id is already expired, do not keep reconnect again.

`void close()`

Close the client, return immediately.

`void sync_close()`

Close the client, return until it is really closed.

`bool connected() const`

Check if client is connected.

### Namespace
`void connect(const std::string& uri)`

Add namespace part `/[any namespaces]` after port, will automatically connect to the namespace you specified.

### Session ID
`std::string const& get_sessionid() const`

Get socket.io session id.

### Message Object
`message` Base class of all message object.

`int_message` message contains a 64-bit integer.

`double_message` message contains a double.

`string_message` message contains a string.

`array_message` message contains a `vector<message::ptr>`.

`object_message` message contains a `map<string,message::ptr>`.

`message::ptr` pointer to `message` object, it will be one of its derived classes, judge by `message.get_flag()`.

All designated constructor of `message` objects is hidden, you need to create message and get the `message::ptr` by `[derived]_message:create()`.

##Example

Simple Console client Login to socket.io [chat room demo](https://github.com/Automattic/socket.io/tree/master/examples/chat).
Find full example file [here](https://github.com/socketio/socket.io-client-cpp/blob/master/examples/Console/main.cpp)
```C++
#define HIGHLIGHT(__O__) std::cout<<"\e[1;31m"<<__O__<<"\e[0m"<<std::endl
#define EM(__O__) std::cout<<"\e[1;30;1m"<<__O__<<"\e[0m"<<std::endl
#include <functional>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <string>
using namespace sio;
using namespace std;
std::mutex _lock;

std::condition_variable_any _cond;
bool connect_finish = false;

class connection_listener
{
    sio::client &handler;

public:
    
    connection_listener(sio::client& h):
    handler(h)
    {
    }
    

    void on_connected()
    {
        _lock.lock();
        _cond.notify_all();
        connect_finish = true;
        _lock.unlock();
    }
    void on_close(client::close_reason const& reason)
    {
        std::cout<<"sio closed "<<std::endl;
        exit(0);
    }
    
    void on_fail()
    {
        std::cout<<"sio failed "<<std::endl;
        exit(0);
    }
};

int main(int argc ,const char* args[])
{

    sio::client h;
    connection_listener l(h);
    h.set_connect_listener(std::bind(&connection_listener::on_connected, &l));
    h.set_close_listener(std::bind(&connection_listener::on_close, &l,std::placeholders::_1));
    h.set_fail_listener(std::bind(&connection_listener::on_fail, &l));
    h.connect("http://127.0.0.1:3000");
    _lock.lock();
    if(!connect_finish)
    {
        _cond.wait(_lock);
    }
    _lock.unlock();
    string nickname;
    while (nickname.length() == 0) {
        HIGHLIGHT("Type your nickname:");
        
        getline(cin, nickname);
    }
    h.bind_event("login", [&](string const& name, message::ptr const& data, bool isAck,message::ptr &ack_resp){
        _lock.lock();
        participants = data->get_map()["numUsers"]->get_int();
        bool plural = participants !=1;
        HIGHLIGHT("Welcome to Socket.IO Chat-\nthere"<<(plural?" are ":"'s ")<< participants<<(plural?" participants":" participant"));
        _cond.notify_all();
        _lock.unlock();
        h.unbind_event("login");
    });
}
```
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
bjam stage --toolset=msvc --with-system --with-date_time --with-thread --with-regex --with-serialization --with-random --stagedir="release" link=static runtime-link=shared threading=multi release
```
For iOS and OSX
Use this [shell](https://github.com/socketio/socket.io-client-cpp/blob/master/examples/iOS/SioChatDemo/boost/boost.sh) to download and build boost completely automattically.

Finally, Add boost source folder to `header search path`, and add static libs to link option.

##License
BSD
