cd build
cmake --build . --config Debug
cmake --build . --config Release
cmake --build . --config RelWithDebInfo
cmake --install . --config Debug --prefix ../../vcpkg/packages/mutils_x64-windows-static-md
cmake --install . --config Release --prefix ../../vcpkg/packages/mutils_x64-windows-static-md
cmake --install . --config RelWithDebInfo --prefix ../../vcpkg/packages/mutils_x64-windows-static-md
cd ..

