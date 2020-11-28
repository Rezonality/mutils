[MUtils](https://github.com/Rezonality/MUtils) - Useful C++
===================================================================================================

![Builds](https://github.com/Rezonality/mutils/workflows/Builds/badge.svg)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/Rezonality/zep/blob/master/LICENSE)
[![codecov](https://codecov.io/gh/Rezonality/MUtils/branch/master/graph/badge.svg)](https://codecov.io/gh/Rezonality/MUtils)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/8bb47022b078477186c6941140e0f62c)](https://www.codacy.com/gh/Rezonality/mutils/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=Rezonality/mutils&amp;utm_campaign=Badge_Grade)
[![Codacy Badge](https://app.codacy.com/project/badge/Coverage/8bb47022b078477186c6941140e0f62c)](https://www.codacy.com/gh/Rezonality/mutils/dashboard?utm_source=github.com&utm_medium=referral&utm_content=Rezonality/mutils&utm_campaign=Badge_Coverage)

This project contains useful things that are often needed in bigger c++ projects.  If something is in here, it helps save time when teaching coding or building apps.  The point of this big static library and associated modules is that when you have built it, you can make apps quickly without figuring out how to compile and get everything to work cross platform (On Windows, Mac and Linux)

This library now uses vcpkg to simplify cross platform building of all the dependent packages.  Running prebuild will create a parallel folder containing all the useful bits.  Some libraries that are not part of vcpkg are still built by MUtils, but the majority are created in the vcpkg.  The following step may take a little time, but is only done once:

``` bash
prebuild.bat OR prebuild.sh
```

The MUtils build is pretty standard, and makes useful shared code, as well as installing various pieces.
It is built like this:

``` bash
config.bat OR config.sh
build_all.bat OR build.sh
```

Now in your project CMakeLists.txt, you can add the following to reference the library and pull in all the header paths.  Note there are no include/library defines; the target_link_libraries is enough to pull all the necessary things in.

``` cmake
find_package(MUtils REQUIRED)
...
target_link_libraries(MyApp PRIVATE MUtils::MUtils)
```

## External Projects
The following external projects are in the library for convenient access.  Look in their folders for more information.  These libraries are all cross platform - you shouldn't be using ones that arent.  There are a couple projects that duplicate functionality; this is mostly to maintain backward compatibility if I've used another version.  Some of these components are installed on the machine when you run 'prebuild.bat/sh'.  Most of them are built into the static MUtils library.

*  Catch2 - better unit testing that gtest
*  Chibi - a scheme implementation for embedding an interpreter
*  Clip - clipboard support
*  Ctti - Compile time type information
*  SDL2 - for windows/UI, as prebuilt package dependency
*  Freetype - for better fonts, as a prebuilt package dependency
*  httplib - a library for making http & https requests
*  OpenSSL - for ssh support (so that httplib can make https calls), as a prebuilt dependency
*  FileWatcher - utilities for watching for file changes 
*  GLM - a math library
*  json - for reading and writing json files
*  lexertk - a simple tokenizer/lexer
*  magic_enum - the missing enum reflection api from c++
*  pfd/tfd - file dialogs
*  zip - creating and loading zip files
*  toml11 & cpptoml - a useful/easy file format for storing configuration
*  soundpipe, dr_libs, miniaudio, kiss_fft - useful libraries for sounds
*  stb - single header utility libraries
*  imgui - a GUI library, including OpenGL components
*  tinydir - for parsing directories/searching for files
*  fmt - a better string formatter
*  tclap - for parsing commandline arguments
*  threadpool - a simple C++ threadpool
*  tracy - an awesome real time profiler
*  mpc - a parser generator that's easy to use
*  gsl-lit - a coding standards library

## Utilties

These are useful snippets of code I've gathered over time.

### Math functions
Functions for random numbers, aligning memory, handling rectangles, etc.

### GL
Compiling shaders, loading textures, creating geometry buffers and loading simple shapes

### Animtation
A simple clock/timer

### Callback
A scheduler that will call a function after a given time

### String
Various utilities to split, tokenize and manipulate strings

### Logger
A really basic logger for outputting messages from an app

### Thread
Utilities for handling futures, and a threadpool for scheduling work

### UI
Useful functions for determining DPI and showing messages
A simple wrapper for SDL/IMGUI to make it easy to build a 2D/3D app using a GUI and OpenGL

### VM
A VM which can take arbitrary variants as its core values.  Useful to execute previously compiled code


