You need have your boost unpacked on your disk, at least staged following modules:

* system
* date_time
* random
* unit_test_framework

Then use following instruction to gen makefile or VS project.
```bash
cmake -DBOOST_LIBRARYDIR=`<your boost static libs path>` -DBOOST_INCLUDEDIR=`<your boost include path>` -DBOOST_VER:STRING=`<your boost version>` -DCMAKE_BUILD_TYPE=Debug ./
```
Then run `make` or open by VS.

For example I've installed boost 1.57.0 at `D:\boost_1_57_0` and staged the static lib at `D\boost_1_57_0\build\lib` then the command should be:
```bash
cmake -DBOOST_LIBRARYDIR=D:\boost_1_57_0\build\lib -DBOOST_INCLUDEDIR=D:\boost_1_57_0 -DBOOST_VER:STRING=1.57.0 -DCMAKE_BUILD_TYPE=Debug ./
```
In this case(Windows) CMake will create a VS project under `./test` folder. Open in VS and run it.