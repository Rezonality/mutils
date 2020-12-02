find_package(freetype CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(gsl-lite CONFIG REQUIRED)
find_package(unofficial-concurrentqueue CONFIG REQUIRED)
find_package(tinyfiledialogs CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(gl3w CONFIG REQUIRED)

# Used Packages external to vcpkg
include(GNUInstallDirs)

set(UTILS_SOURCES
    ${MUTILS_ROOT}/src/CMakeLists.txt
    ${MUTILS_ROOT}/src/algorithm/container_utils.cpp
    ${MUTILS_ROOT}/src/callback/callback.cpp
    ${MUTILS_ROOT}/src/chibi/chibi.cpp
    ${MUTILS_ROOT}/src/compile/compile_messages.cpp
    ${MUTILS_ROOT}/src/compile/meta_tags.cpp
    ${MUTILS_ROOT}/src/file/file.cpp
    ${MUTILS_ROOT}/src/file/runtree.cpp
    ${MUTILS_ROOT}/src/file/toml_utils.cpp
    ${MUTILS_ROOT}/src/geometry/indexer.cpp
    ${MUTILS_ROOT}/src/geometry/standard_shapes.cpp
    ${MUTILS_ROOT}/src/geometry/vertex_spacial_sort.cpp
    ${MUTILS_ROOT}/src/gl/gl_buffer.cpp
    ${MUTILS_ROOT}/src/gl/gl_shader.cpp
    ${MUTILS_ROOT}/src/gl/gl_texture.cpp
    ${MUTILS_ROOT}/src/math/math_utils.cpp
    ${MUTILS_ROOT}/src/string/murmur_hash.cpp
    ${MUTILS_ROOT}/src/string/string_utils.cpp
    ${MUTILS_ROOT}/src/thread/mempool.cpp
    ${MUTILS_ROOT}/src/time/profiler.cpp
    ${MUTILS_ROOT}/src/time/time_provider.cpp
    ${MUTILS_ROOT}/src/time/timeline.cpp
    ${MUTILS_ROOT}/src/time/timer.cpp
    ${MUTILS_ROOT}/src/ui/colors.cpp
    ${MUTILS_ROOT}/src/ui/theme.cpp
    ${MUTILS_ROOT}/src/ui/dpi.cpp
    ${MUTILS_ROOT}/src/ui/fbo.cpp
    ${MUTILS_ROOT}/src/ui/imgui_extras.cpp
    ${MUTILS_ROOT}/src/ui/sdl_imgui_starter.cpp
    ${MUTILS_ROOT}/src/ui/ui_manager.cpp

    ${MUTILS_ROOT}/include/mutils/algorithm/container_utils.h
    ${MUTILS_ROOT}/include/mutils/algorithm/ringiterator.h
    ${MUTILS_ROOT}/include/mutils/chibi/chibi.h
    ${MUTILS_ROOT}/include/mutils/compile/compile_messages.h
    ${MUTILS_ROOT}/include/mutils/compile/meta_tags.h
    ${MUTILS_ROOT}/include/mutils/device/IDevice.h
    ${MUTILS_ROOT}/include/mutils/device/IDeviceBuffer.h
    ${MUTILS_ROOT}/include/mutils/file/runtree.h
    ${MUTILS_ROOT}/include/mutils/file/toml_utils.h
    ${MUTILS_ROOT}/include/mutils/geometry/geometry.h
    ${MUTILS_ROOT}/include/mutils/geometry/icosahedron.h
    ${MUTILS_ROOT}/include/mutils/geometry/icosphere.h
    ${MUTILS_ROOT}/include/mutils/geometry/indexer.h
    ${MUTILS_ROOT}/include/mutils/geometry/sphere.h
    ${MUTILS_ROOT}/include/mutils/geometry/standard_shapes.h
    ${MUTILS_ROOT}/include/mutils/geometry/vertex_spacial_sort.h
    ${MUTILS_ROOT}/include/mutils/gl/gl_buffer.h
    ${MUTILS_ROOT}/include/mutils/gl/gl_shader.h
    ${MUTILS_ROOT}/include/mutils/gl/gl_texture.h
    ${MUTILS_ROOT}/include/mutils/thread/thread_utils.h
    ${MUTILS_ROOT}/include/mutils/thread/mempool.h
    ${MUTILS_ROOT}/include/mutils/ui/colors.h
    ${MUTILS_ROOT}/include/mutils/ui/theme.h
    ${MUTILS_ROOT}/include/mutils/ui/dpi.h
    ${MUTILS_ROOT}/include/mutils/ui/fbo.h
    ${MUTILS_ROOT}/include/mutils/ui/imgui_extras.h
    ${MUTILS_ROOT}/include/mutils/ui/sdl_imgui_starter.h
    ${MUTILS_ROOT}/include/mutils/ui/ui_manager.h
    ${MUTILS_ROOT}/include/mutils/vm/vm.h
    )

set(CLIP_SOURCE
    ${M3RDPARTY_DIR}/clip/clip.cpp
    ${M3RDPARTY_DIR}/clip/image.cpp
    )

if(WIN32)
    LIST(APPEND CLIP_SOURCE
        ${M3RDPARTY_DIR}/clip/clip_win.cpp)
endif()
if (UNIX)
    if (APPLE)
        LIST(APPEND CLIP_SOURCE
            ${M3RDPARTY_DIR}/clip/clip_osx.mm)
    else()
        LIST(APPEND CLIP_SOURCE
            ${M3RDPARTY_DIR}/clip/clip_x11.cpp)
    endif() # APPLE
endif() # UNIX

# Tracy
set(TRACY_SOURCE
    ${M3RDPARTY_DIR}/tracy/TracyClient.cpp)

set (JANET_SOURCE
    ${M3RDPARTY_DIR}/janet/janet.c)

set (THREADPOOL_SOURCE
    # SEXP
    ${M3RDPARTY_DIR}/threadpool/threadpool.h
    )

set (IMGUIFILEDIALOG_SOURCE
    ${M3RDPARTY_DIR}/imguifiledialog/ImGuiFileDialog/ImGuiFileDialog.cpp
    ${M3RDPARTY_DIR}/imguifiledialog/ImGuiFileDialog/ImGuiFileDialog.h
    ${M3RDPARTY_DIR}/imguifiledialog/CustomFont.cpp
    ${M3RDPARTY_DIR}/imguifiledialog/CustomFont.h
    ${M3RDPARTY_DIR}/imguifiledialog/ImGuiFileDialog/ImGuiFileDialogConfig.h
    )

set (CHIBI_SCHEME_SOURCE
    # SEXP
    ${M3RDPARTY_DIR}/chibi/gc.c
    ${M3RDPARTY_DIR}/chibi/gc_heap.c
    ${M3RDPARTY_DIR}/chibi/sexp.c
    ${M3RDPARTY_DIR}/chibi/bignum.c
    ${M3RDPARTY_DIR}/chibi/opcodes.c
    ${M3RDPARTY_DIR}/chibi/vm.c
    ${M3RDPARTY_DIR}/chibi/eval.c
    ${M3RDPARTY_DIR}/chibi/simplify.c)

set (ORCA_SOURCE
    ${M3RDPARTY_DIR}/orca-c/field.c
    ${M3RDPARTY_DIR}/orca-c/gbuffer.c
    #${M3RDPARTY_DIR}/orca-c/osc_out.c
    ${M3RDPARTY_DIR}/orca-c/sim.c
    #${M3RDPARTY_DIR}/orca-c/sysmisc.c
    ${M3RDPARTY_DIR}/orca-c/vmio.c
    ${M3RDPARTY_DIR}/orca-c/vmio.h)

set(default_module_path
    ""
    #"${CMAKE_INSTALL_PREFIX}/${thePrefix}${pathsep}${CMAKE_INSTALL_PREFIX}/bin"
    )

# Special step for chibi
if (NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/include/chibi/install.h)

file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/chibi)
file(READ ${MUTILS_ROOT}/m3rdparty/chibi/RELEASE release)
string(STRIP ${release} release)

if(WIN32)
    set(soext ".dll")
    set(platform "windows")
else()
    set(soext ".so")
    set(platform "unknown")
endif()

file(WRITE
    ${CMAKE_CURRENT_BINARY_DIR}/include/chibi/install.h
    "#define sexp_so_extension \"${soext}\"
    #define sexp_default_module_path \"${default_module_path}\"
    #define sexp_platform \"${platform}\"
    #define sexp_version \"\"
    #define sexp_release_name \"${release}\""
    )

endif()

set (MPC_SOURCE
    ${M3RDPARTY_DIR}/mpc/mpc.c)

SET(SOUNDPIPE_DIR ${MUTILS_ROOT}/m3rdparty/soundpipe)
SET(SOUNDPIPE_EXTENSIONS_DIR ${MUTILS_ROOT}/m3rdparty/soundpipe_extensions)

file(GLOB_RECURSE SOUNDPIPE_SOURCE "${SOUNDPIPE_DIR}/modules/*.c")
list (REMOVE_ITEM SOUNDPIPE_SOURCE "${SOUNDPIPE_DIR}/modules/diskin.c")            
LIST(APPEND SOUNDPIPE_SOURCE
    ${SOUNDPIPE_DIR}/lib/dr_wav/dr_wav.c
    ${SOUNDPIPE_DIR}/lib/inih/ini.c
    ${SOUNDPIPE_EXTENSIONS_DIR}/modules/oscmorph2d.c)

# Create the library
add_library(MUtils STATIC 
    ${CHIBI_SCHEME_SOURCE}
    ${CLIP_SOURCE}
    ${IMGUIFILEDIALOG_SOURCE}
    ${JANET_SOURCE}
    ${MPC_SOURCE}
    ${ORCA_SOURCE}
    ${SOUNDPIPE_SOURCE}
    ${THREADPOOL_SOURCE}
    ${TRACY_SOURCE}
    ${UTILS_SOURCES}
    )

target_compile_definitions(MUtils
    PUBLIC
    NO_LIBSNDFILE
    _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING)

if (WIN32)
    # Sound pipe plays fast and loose with float/double conversions and other things.
    # To be fair, this is probably its inherited Csound code.
    # Certainly on windows there are a few compile warnings which I'm ignoring for now
    set_target_properties(MUtils PROPERTIES COMPILE_FLAGS "/wd4244 /wd4305 /wd4005 /wd4273 /wd4018 /wd4267 /wd4129")
endif()

add_library(MUtils::MUtils ALIAS MUtils)

find_path(CPPTOML_INCLUDE_DIRS "cpptoml.h")
find_path(TOML11_INCLUDE_DIRS "toml.hpp")
find_path(STB_INCLUDE_DIRS "stb.h")
find_path(TINYDIR_INCLUDE_DIRS "tinydir.h")
find_path(GLM_INCLUDE_DIRS glm/glm.hpp)

target_include_directories(MUtils
    PRIVATE
    $<BUILD_INTERFACE:${MUTILS_ROOT}/m3rdparty>
    ${MUTILS_ROOT}/m3rdparty/janet
    ${MUTILS_ROOT}/include
    ${MUTILS_ROOT}/include/mutils
    ${CMAKE_CURRENT_BINARY_DIR}/include
    PUBLIC 
    ${CPPTOML_INCLUDE_DIRS}
    ${TOML11_INCLUDE_DIRS}
    ${STB_INCLUDE_DIRS}
    ${TINYDIR_INCLUDE_DIRS}
    ${GLM_INCLUDE_DIRS}
    $<BUILD_INTERFACE:${MUTILS_ROOT}/m3rdparty>
    $<BUILD_INTERFACE:${MUTILS_ROOT}/m3rdparty/janet>
    $<BUILD_INTERFACE:${MUTILS_ROOT}/m3rdparty/tfd>
    $<BUILD_INTERFACE:${MUTILS_ROOT}/m3rdparty/chibi/include>
    $<BUILD_INTERFACE:${MUTILS_ROOT}/m3rdparty/ctti/include>
    $<BUILD_INTERFACE:${SOUNDPIPE_DIR}/lib/dr_wav>
    $<BUILD_INTERFACE:${SOUNDPIPE_DIR}/lib/faust>
    $<BUILD_INTERFACE:${SOUNDPIPE_DIR}/lib/inih>
    $<BUILD_INTERFACE:${SOUNDPIPE_DIR}/lib/openlpc>
    $<BUILD_INTERFACE:${SOUNDPIPE_DIR}/lib/kissfft>
    $<BUILD_INTERFACE:${SOUNDPIPE_DIR}/lib/spa>
    $<BUILD_INTERFACE:${SOUNDPIPE_DIR}/h>
    $<BUILD_INTERFACE:${SOUNDPIPE_EXTENSIONS_DIR}>

    $<INSTALL_INTERFACE:include/clip>
    $<INSTALL_INTERFACE:include/tracy>
    $<INSTALL_INTERFACE:include/orca-c>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/nod>
    $<INSTALL_INTERFACE:include/soundpipe/h>
)

target_link_libraries(MUtils PUBLIC
    SDL2::SDL2-static
    imgui::imgui
    gsl::gsl-lite-v1
    fmt::fmt-header-only
    unofficial::gl3w::gl3w
    unofficial::concurrentqueue::concurrentqueue
    tinyfiledialogs::tinyfiledialogs
    freetype
    )

if (TARGET_LINUX)
message(STATUS "Adding stdc++fs for experimental filesystem")
target_link_libraries(MUtils PUBLIC
    stdc++fs
    xcb
    )
endif()

if (WIN32)
target_link_libraries(MUtils PUBLIC
    ws2_32)
endif()
# Set locations for components
set_target_properties(MUtils PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )

# Install the binary
install(TARGETS MUtils
    EXPORT mutils-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${LIBLEGACY_INCLUDE_DIRS}
    )

# CMake
install(FILES ${MUTILS_ROOT}/cmake/utils.cmake
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cmake
    )

# Install the include files
install(DIRECTORY ${MUTILS_ROOT}/include/mutils
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/nod/include/nod
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/orca-c
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/clip
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/janet
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/lexertk
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/threadpool
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/chibi/include/chibi
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(FILES ${MUTILS_ROOT}/m3rdparty/chibi/lib/init-7.scm
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/chibi
    )

install(FILES ${MUTILS_ROOT}/m3rdparty/chibi/lib/meta-7.scm
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/chibi
    )

# Note: We keep the necessary includes seperate for clean install
install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/tracy
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tracy
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/imguifiledialog/ImGuiFileDialog
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/chibi/install.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/chibi)

install(FILES ${MUTILS_ROOT}/m3rdparty/miniaudio/miniaudio.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/miniaudio
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/soundpipe/h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/soundpipe
    )

install(FILES ${MUTILS_ROOT}/m3rdparty/soundpipe_extensions/soundpipeextension.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/soundpipe
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/soundpipe/lib
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/soundpipe
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/ctti/include/ctti
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/pfd
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

source_group ("Chibi" FILES ${CHIBI_SCHEME_SOURCE})
source_group ("Orca" FILES ${ORCA_SOURCE})
source_group ("Janet" FILES ${JANET_SOURCE})
source_group ("mpc" FILES ${MPC_SOURCE})
source_group ("Tracy" FILES ${TRACY_SOURCE})
source_group ("Clip" FILES ${CLIP_SOURCE})
source_group ("MUtils\\Timing" .*time.*)
source_group ("MUtils\\String" .*string.*)
source_group ("MUtils\\Math" .*math.*)
source_group ("MUtils\\Geometry" .*geometry.*)
source_group ("MUtils\\File" .*file.*)
source_group ("MUtils\\Compile" .*compile.*)
source_group ("MUtils\\Chibi" .*chibi.*)
source_group ("MUtils\\Callback" .*callback.*)
source_group ("MUtils\\GL" .*gl.*)
source_group ("MUtils\\Thread" .*thread.*)
source_group ("MUtils\\UI" .*ui.*)
source_group ("MUtils\\VM" .*vm.*)
source_group ("MUtils\\Device" .*device.*)
source_group ("MUtils\\Algorithm" .*algorithm.*)

#source_group ("Utils" FILES ${UTILS_SOURCES})
source_group ("SoundPipe" FILES ${SOUNDPIPE_SOURCE})
source_group ("ThreadPool" FILES ${THREADPOOL_SOURCE})

