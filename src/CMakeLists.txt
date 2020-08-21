if (NOT WIN32)
find_package(Freetype REQUIRED)
endif()

set(UTILS_SOURCES
    ${MUTILS_ROOT}/src/CMakeLists.txt
    ${MUTILS_ROOT}/src/algorithm/containerutils.cpp
    ${MUTILS_ROOT}/src/callback/callback.cpp
    ${MUTILS_ROOT}/src/chibi/chibi.cpp
    ${MUTILS_ROOT}/src/compile/compile_messages.cpp
    ${MUTILS_ROOT}/src/compile/meta_tags.cpp
    ${MUTILS_ROOT}/src/file/file.cpp
    ${MUTILS_ROOT}/src/file/runtree.cpp
    ${MUTILS_ROOT}/src/geometry/indexer.cpp
    ${MUTILS_ROOT}/src/geometry/standard_shapes.cpp
    ${MUTILS_ROOT}/src/geometry/vertex_spacial_sort.cpp
    ${MUTILS_ROOT}/src/gl/gl_buffer.cpp
    ${MUTILS_ROOT}/src/gl/gl_shader.cpp
    ${MUTILS_ROOT}/src/gl/gl_texture.cpp
    ${MUTILS_ROOT}/src/math/mathutils.cpp
    ${MUTILS_ROOT}/src/string/murmur_hash.cpp
    ${MUTILS_ROOT}/src/string/stringutils.cpp
    ${MUTILS_ROOT}/src/string/tomlutils.cpp
    ${MUTILS_ROOT}/src/thread/mempool.cpp
    ${MUTILS_ROOT}/src/time/profiler.cpp
    ${MUTILS_ROOT}/src/time/time_provider.cpp
    ${MUTILS_ROOT}/src/time/timeline.cpp
    ${MUTILS_ROOT}/src/time/timer.cpp
    ${MUTILS_ROOT}/src/ui/colors.cpp
    ${MUTILS_ROOT}/src/ui/dpi.cpp
    ${MUTILS_ROOT}/src/ui/fbo.cpp
    ${MUTILS_ROOT}/src/ui/imgui_extras.cpp
    ${MUTILS_ROOT}/src/ui/sdl_imgui_starter.cpp
    ${MUTILS_ROOT}/src/ui/ui_manager.cpp

    ${MUTILS_ROOT}/include/mutils/algorithm/containerutils.h
    ${MUTILS_ROOT}/include/mutils/algorithm/ringiterator.h
    ${MUTILS_ROOT}/include/mutils/chibi/chibi.h
    ${MUTILS_ROOT}/include/mutils/compile/compile_messages.h
    ${MUTILS_ROOT}/include/mutils/compile/meta_tags.h
    ${MUTILS_ROOT}/include/mutils/device/IDevice.h
    ${MUTILS_ROOT}/include/mutils/device/IDeviceBuffer.h
    ${MUTILS_ROOT}/include/mutils/file/runtree.h
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
    ${MUTILS_ROOT}/include/mutils/ui/colors.h
    ${MUTILS_ROOT}/include/mutils/ui/dpi.h
    ${MUTILS_ROOT}/include/mutils/ui/fbo.h
    ${MUTILS_ROOT}/include/mutils/ui/imgui_extras.h
    ${MUTILS_ROOT}/include/mutils/ui/sdl_imgui_starter.h
    ${MUTILS_ROOT}/include/mutils/ui/ui_manager.h
    ${MUTILS_ROOT}/include/mutils/vm/vm.h
    )

# Zip compression/decompression support
file(GLOB ZIP_SOURCE 
    ${M3RDPARTY_DIR}/zip/src/zip.c
    ${M3RDPARTY_DIR}/zip/src/zip.h
    ${M3RDPARTY_DIR}/zip/src/miniz.h
    )

# File watcher
set (FILEWATCHER_SOURCE
    ${M3RDPARTY_DIR}/FileWatcher/FileWatcher.cpp
    ${M3RDPARTY_DIR}/FileWatcher/FileWatcher.h)

if (WIN32)
    Set (FILEWATCHER_SOURCE
        ${FILEWATCHER_SOURCE}
        ${M3RDPARTY_DIR}/FileWatcher/FileWatcherWin32.cpp
        )
else()
    SET (FILEWATCHER_SOURCE
        ${FILEWATCHER_SOURCE}
        ${M3RDPARTY_DIR}/FileWatcher/FileWatcherLinux.cpp
        )
endif()

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

# Tiny File Dialogs
set(TFD_SOURCE
    ${M3RDPARTY_DIR}/tfd/tinyfiledialogs.c)

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

#FMT
SET (FMT_SOURCE 
    ${M3RDPARTY_DIR}/fmt/format.cc)

# KissFFT
set (KISFFT_SOURCE
    ${M3RDPARTY_DIR}/kiss_fft/kiss_fft.c)

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

SET(IMGUI_DIR ${MUTILS_ROOT}/m3rdparty/imgui)
SET(IMGUI_SOURCE
    ${IMGUI_DIR}/examples/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/examples/imgui_impl_sdl.cpp
    ${IMGUI_DIR}/examples/libs/gl3w/GL/gl3w.c
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/misc/freetype/imgui_freetype.cpp
    ${IMGUI_DIR}/imgui.cpp)

SET(NANOVG_DIR ${MUTILS_ROOT}/m3rdparty/nanovg)
SET(NANOVG_SOURCE
    ${NANOVG_DIR}/src/nanovg.c
    ${NANOVG_DIR}/src/nanovg.h
    ${NANOVG_DIR}/src/nanovg_gl.h
    ${NANOVG_DIR}/src/fontstash.h
    ${NANOVG_DIR}/src/nanovg_gl_utils.h
    ${NANOVG_DIR}/src/stb_image.h
    ${NANOVG_DIR}/src/stb_truetype.h
)

# Create the library
add_library(MUtils STATIC 
    ${UTILS_SOURCES}
    ${TRACY_SOURCE}
    ${JANET_SOURCE}
    ${FMT_SOURCE}
    ${KISFFT_SOURCE}
    ${MPC_SOURCE}
    ${IMGUI_SOURCE}
    ${NANOVG_SOURCE}
    ${SOUNDPIPE_SOURCE}
    ${SDL2_SOURCE}
    ${TFD_SOURCE}
    ${CHIBI_SCHEME_SOURCE}
    ${ORCA_SOURCE}
    ${FILEWATCHER_SOURCE}
    ${ZIP_SOURCE}
    ${CLIP_SOURCE}
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

target_include_directories(MUtils
    PRIVATE
    $<BUILD_INTERFACE:${MUTILS_ROOT}/m3rdparty>
    ${MUTILS_ROOT}/m3rdparty/tinydir
    ${MUTILS_ROOT}/m3rdparty/janet
    ${MUTILS_ROOT}/m3rdparty/ctti/include
    ${MUTILS_ROOT}/include
    ${MUTILS_ROOT}/include/mutils
    ${IMGUI_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/include
    PUBLIC 
    $<BUILD_INTERFACE:${MUTILS_ROOT}/m3rdparty>
    $<BUILD_INTERFACE:${MUTILS_ROOT}/m3rdparty/glm>
    $<BUILD_INTERFACE:${MUTILS_ROOT}/m3rdparty/janet>
    $<BUILD_INTERFACE:${MUTILS_ROOT}/m3rdparty/tfd>
    $<BUILD_INTERFACE:${MUILTS_ROOT}/m3rdparty/ctti/include>
    $<BUILD_INTERFACE:${MUTILS_ROOT}/m3rdparty/chibi/include>
    $<BUILD_INTERFACE:${FREETYPE_INCLUDE_DIRS}>

    $<BUILD_INTERFACE:${SOUNDPIPE_DIR}/lib/dr_wav>
    $<BUILD_INTERFACE:${SOUNDPIPE_DIR}/lib/faust>
    $<BUILD_INTERFACE:${SOUNDPIPE_DIR}/lib/inih>
    $<BUILD_INTERFACE:${SOUNDPIPE_DIR}/lib/kissfft>
    $<BUILD_INTERFACE:${SOUNDPIPE_DIR}/lib/openlpc>
    $<BUILD_INTERFACE:${SOUNDPIPE_DIR}/lib/spa>
    $<BUILD_INTERFACE:${SOUNDPIPE_DIR}/h>
    $<BUILD_INTERFACE:${SOUNDPIPE_EXTENSIONS_DIR}>
    $<BUILD_INTERFACE:${IMGUI_DIR}/examples/libs/gl3w>
    $<BUILD_INTERFACE:${IMGUI_DIR}/misc/freetype>

    $<INSTALL_INTERFACE:include/rtaudio>
    $<INSTALL_INTERFACE:include/openssl>
    $<INSTALL_INTERFACE:include/toml11>
    $<INSTALL_INTERFACE:include/utfcpp/source>
    $<INSTALL_INTERFACE:include/clip>
    $<INSTALL_INTERFACE:include/tracy>
    $<INSTALL_INTERFACE:include/orca-c>
    $<INSTALL_INTERFACE:include/nanovg>
    $<INSTALL_INTERFACE:include>
    $<INSTALL_INTERFACE:include/imgui>
    $<INSTALL_INTERFACE:include/imgui/misc/freetype>
    $<INSTALL_INTERFACE:include/glm>
    $<INSTALL_INTERFACE:include/nod>
    $<INSTALL_INTERFACE:include/soundpipe/h>
    $<INSTALL_INTERFACE:include/ctti>
)

target_link_libraries(MUtils 
    PRIVATE
    SDL2::SDL2-static)

# Windows find_package(freetype) is broken.
if (WIN32)
    target_include_directories(MUtils
        PRIVATE
        ${MUTILS_ROOT}/m3rdparty/packages/build/freetype-install/include/freetype2
        PUBLIC
        $<INSTALL_INTERFACE:include/freetype2>
        )
    target_link_libraries(MUtils
        PUBLIC
        # Wire in the library full path as a transitive dependency for freetype on windows!
        ${MUTILS_ROOT}/m3rdparty/packages/build/freetype-install/lib/freetype$<$<CONFIG:DEBUG>:d>.lib
        Ws2_32)
else()
    target_include_directories(MUtils
        PUBLIC
        $<INSTALL_INTERFACE:${FREETYPE_INCLUDE_DIRS}>
        )
    target_link_libraries(MUtils 
        PRIVATE
        ${FREETYPE_LIBRARIES}
        )
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

if (WIN32)
    install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/packages/build/freetype-install/include
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/freetype2
        )
    install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/packages/build/freetype-install/lib
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/freetype2
        )
endif()

# CMake
install(FILES ${MUTILS_ROOT}/cmake/utils.cmake
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/cmake
    )

if (WIN32)
install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/packages/build/rtaudio-install/include
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rtaudio
    )
install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/packages/build/rtaudio-install/lib
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/rtaudio
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/packages/build/openssl-install/include/openssl
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/packages/build/openssl-install/lib
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/openssl
    )
endif()

# Install the include files
install(DIRECTORY ${MUTILS_ROOT}/include/mutils
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/FileWatcher
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/toml11
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/nod/include/nod
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/orca-c
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/utfcpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(FILES ${MUTILS_ROOT}/m3rdparty/magic_enum/include/magic_enum.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/magic_enum
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/clip
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/json/single_include/nlohmann
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/date/include/date
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/glm
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/janet
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/fmt
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/stb
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY 
        ${MUTILS_ROOT}/m3rdparty/libcuckoo/libcuckoo
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/kiss_fft
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/httplib
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/lexertk
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/cpptoml
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/threadpool
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/tclap
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(FILES ${MUTILS_ROOT}/m3rdparty/zip/src/zip.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/zip
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/imgui
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

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/chibi/install.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/chibi)

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/gsl
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(FILES 
    ${MUTILS_ROOT}/m3rdparty/imgui/examples/libs/gl3w/GL/gl3w.h
    ${MUTILS_ROOT}/m3rdparty/imgui/examples/libs/gl3w/GL/glcorearb.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/GL
    )

install(FILES ${NANOVG_SOURCE}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nanovg
    )

install(FILES 
    ${MUTILS_ROOT}/m3rdparty/tfd/tinyfiledialogs.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tfd
    )

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

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/concurrentqueue
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/catch2
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

install(DIRECTORY ${MUTILS_ROOT}/m3rdparty/pfd
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

source_group ("Utils" FILES ${UTILS_SOURCES})
source_group ("ImGui" FILES ${IMGUI_SOURCE})
source_group ("SoundPipe" FILES ${SOUNDPIPE_SOURCE})

