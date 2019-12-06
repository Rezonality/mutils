cmake_minimum_required(VERSION 3.2)

message(STATUS " CMakeLists: SDL2")

project(PackageBuilder)

# For travis, we need sudo to install the package if it is a dependent from another project build
if (WIN32)
    set (INST_CMD COMMAND ${CMAKE_COMMAND} --build . --config Debug --target install && ${CMAKE_COMMAND} --build . --config Release --target install)
else()
    set (INST_CMD COMMAND sudo ${CMAKE_COMMAND} --build . --config Debug --target install && sudo ${CMAKE_COMMAND} --build . --config Release --target install)
endif()

INCLUDE(ExternalProject)
ExternalProject_Add(
    sdl2
    URL https://www.libsdl.org/release/SDL2-2.0.10.zip
    CMAKE_ARGS -DCMAKE_STATIC=ON 
    INSTALL_COMMAND ${INST_CMD}
)

# TODO: These rely on Master.  Should find a tag/commit and sync to that.
# Note though that RtAudio releases don't work, but current master does!

if (WIN32)
ExternalProject_Add(
    freetype
    GIT_REPOSITORY "https://github.com/aseprite/freetype2.git"
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    GIT_TAG VER-2-10-0
    CMAKE_ARGS -DFT2_BUILD_LIBRARY=ON -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_LIST_DIR}/build/freetype-install
    INSTALL_COMMAND ${INST_CMD}
    )
else()
ExternalProject_Add(
    freetype
    GIT_REPOSITORY "https://github.com/aseprite/freetype2.git"
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    GIT_TAG VER-2-10-0
    CMAKE_ARGS -DFT2_BUILD_LIBRARY=ON 
    INSTALL_COMMAND ${INST_CMD}
    )
endif()

ExternalProject_Add(
    OpenSSL
    GIT_REPOSITORY https://github.com/cmaughan/openssl-cmake.git
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    GIT_TAG 1.1.1b
    INSTALL_DIR ${CMAKE_CURRENT_LIST_DIR}/build/openssl-install
    CMAKE_ARGS -DWITH_APPS=OFF -DCMAKE_DEBUG_POSTFIX=-debug -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_COMMAND ${INST_CMD}
)

ExternalProject_Add(
    RtAudio
    GIT_REPOSITORY "https://github.com/thestk/rtaudio.git"
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    GIT_TAG 5.1.0
    INSTALL_DIR ${CMAKE_CURRENT_LIST_DIR}/build/rtaudio-install
    CMAKE_ARGS -DRTAUDIO_BUILD_STATIC_LIBS=ON -DRTAUDIO_BUILD_TESTING=OFF -DCMAKE_DEBUG_POSTFIX=-debug -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
    INSTALL_COMMAND ${INST_CMD}
)

add_library(PackageBuilder STATIC builder.cpp)
add_dependencies(PackageBuilder sdl2 freetype RtAudio OpenSSL)

