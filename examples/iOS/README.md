#SIOClient iOS chat room demo
## What is this
This is a iOS project run on xcode 6, you can use this single view application chatting in the official [chat room demo](https://github.com/Automattic/socket.io/tree/master/examples/chat).

## How to setup
Suppose you're using your Mac's shell, under your workspace folder, run
```shell
git clone --recurse-submodules https://github.com/socketio/socket.io-client-cpp.git
```
Step in the demo folder
```shell
cd ./socket.io-client-cpp/examples/iOS/SioChatDemo
```
you will see a shell script named `boost.sh` under folder `boost`,Run
```shell
cd ./boost
bash ./boost.sh
```
Please stand by with patient, this step will take about one or two hours depends on your network.
When done, open `SioChatDemo.xcodeproj` file in the parent folder with xcode.
Just compile and run the `SioChatDemo` target.
Now, if you have your chat room server run on your local machine, you can chat with device to device or device to web.

## Use sioclient as static lib on iOS
There's a target named `sioclient` in the Demo project, That is the exactly right config for buiding the `sioclient` as a static library on iOS. 
With the static library file `libsioclient.a` and two exported headers `sio_client.h` and `sio_message.h`, you won't need to config anything again and again in your integrating projects.

## About the `boost.sh`
The `boost.sh` is copied from [boostmake_ios](https://github.com/alist/boostmake_ios),it is worked on my machine for boost 1.55.0
there're lot's versions of boost build shells, you can choose what you like.

