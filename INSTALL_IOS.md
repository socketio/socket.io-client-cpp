## iOS

### Option 1: Create your static library or download precompiled version

Follow the recent tutorial under https://github.com/kim-company/socket.io-client-cpp-ios-static to build from scrattch or download precompiled versions.  

### Option 2: Cocoapods
__Note__: this is the outdated version 1.6.1. 
```
pod 'SocketIO-Client-CPP'
```

### Notes
* See the [iOS example project](examples/iOS) for how to integrate the rest.
* Since commit https://github.com/socketio/socket.io-client-cpp/commit/af68bf3067ab45dc6a53261284e0da9afd21b636 the boost library is not needed anymore. 
