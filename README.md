[MUtils](https://github.com/cmaughan/MUtils) - Useful C++
===================================================================================================

[![Build Status](https://travis-ci.org/cmaughan/MUtils.svg?branch=master)](https://travis-ci.org/cmaughan/MUtils)
[![Build status](https://ci.appveyor.com/api/projects/status/bwyabd9j6j77mes9?svg=true)](https://ci.appveyor.com/project/cmaughan/MUtils)
[![GitHub license](https://img.shields.io/badge/license-MIT-blue.svg)](https://github.com/cmaughan/zep/blob/master/LICENSE)
[![codecov](https://codecov.io/gh/cmaughan/MUtils/branch/master/graph/badge.svg)](https://codecov.io/gh/cmaughan/MUtils)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/ea66b760a217428c996b131bc183072f)](https://www.codacy.com/app/cmaughan/MUtils?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=cmaughan/MUtils&amp;utm_campaign=Badge_Grade)

This project contains useful things that I usually need in c++ projects.  If code is in here, I use it often and I don't expect it to change much.
You can use it in your own projects by first building and installing it using cmake, then referencing it in other packages.

First, you need to build the dependent packages - which will install SDL on your system.
```
prebuild.bat OR prebuild.sh
```

Then build and install the MUtils package:
```
config.bat OR config.sh
build_all.bat OR build.sh
```

Now in your project CMakeLists.txt, you can add the following to reference the library and pull in all the header paths:

```
find_package(MUtils REQUIRED)
...
target_link_libraries(MyApp PRIVATE MUtils::MUtils)
```

## External Projects
The following external projects are in the library for convenient access

* SDL2 (as a prebuilt package dependency)
* GLM (a math library)
* cpptoml (a useful/easy file format for storing configuration)
* soundpipe, dr_libs, miniaudio (useful libraries for playing sounds)
* stb (useful single header files)
* imgui (a rendering library, including OpenGL components)
* tinydir (for parsing directories)
* fmt (a better string formatter)
* kiss_fft (a fast fourier transform)
* tclap (for parsing commandline arguments)
* threadpool (a simple C++ threadpool helper) 
* tracy (an awesome real time profiler)
* mpc (a parser generator that's easy to use)
* gsl-lit (a coding standards library)

## Utilties
    
### Math functions
Functions for random numbers, aligning memory, handling rectangles, etc.

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


