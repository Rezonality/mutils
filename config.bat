set CURRENT_DIR=%CD%
mkdir build > nul
cd build
cmake -G "Visual Studio 16 2019" ..\
cd "%CURRENT_DIR%"

