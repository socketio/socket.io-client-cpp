for /d %%i in (".\*") do if /i not "%%i"=="build.bat" rd /s /q "%%i"
for %%i in (*.*) do if not "%%i"=="build.bat" del /q "%%i"
REM cmake -G "Visual Studio 16 2019" -A x64 -DBOOST_ROOT:STRING="C:\Users\Administrator\git\boost_1_75_0" -DBOOST_VER:STRING="1.75.0" ../
CALL CALL cmake -G "Visual Studio 16 2019" -A x64 -DOPENSSL_ROOT_DIR="C:\Program Files\OpenSSL-Win64" -DBOOST_INCLUDEDIR="C:\Users\Administrator\git\boost_1_75_0" -DBOOST_LIBRARYDIR="C:\Users\Administrator\git\boost_1_75_0\stage\lib" -DBOOST_VER:STRING="1.75.0" ../
pause