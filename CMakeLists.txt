cmake_minimum_required(VERSION 3.2)

message(STATUS " CMakeLists: MUtils")

# Global Options
option(BUILD_TESTS "Build Tests" ON)

# Global Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_DEBUG_POSTFIX "-debug")
set(CMAKE_RELWITHDEBINFO_POSTFIX "-reldbg")

if (WIN32)
    set(VCPKG_TARGET_TRIPLET x64-windows-static-md)
endif()

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

project(MUtils
    LANGUAGES CXX C
    VERSION 0.5.0
    )

# config_app.h checks
# This makes a config_shared.h file which can be included for system settings
#  Must be after setting up the platforms
set(MUTILS_ROOT ${CMAKE_CURRENT_LIST_DIR})
set(M3RDPARTY_DIR ${CMAKE_CURRENT_LIST_DIR}/m3rdparty)
set(MUTILS_INCLUDE_DIR include)
set(MUTILS_LIB_DIR lib)
include(cmake/all.cmake)
include(cmake/utils.cmake)

# The library
include(src/CMakeLists.txt)

# The tests
include(tests/CMakeLists.txt)

# Make the CMake bits that ensure find_package does the right thing
install(EXPORT mutils-targets
    FILE mutils-targets.cmake
    NAMESPACE MUtils::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mutils
    )

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${MUTILS_ROOT}/cmake/mutils-config.cmake.in
    ${CMAKE_BINARY_DIR}/cmake/mutils-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mutils
    PATH_VARS MUTILS_INCLUDE_DIR
    PATH_VARS MUTILS_LIB_DIR
    )

write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/cmake/mutils-config-version.cmake
    VERSION 1.0
    COMPATIBILITY AnyNewerVersion
    )

install(
    FILES
    ${CMAKE_BINARY_DIR}/cmake/mutils-config.cmake
    ${CMAKE_BINARY_DIR}/cmake/mutils-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/mutils
    )

