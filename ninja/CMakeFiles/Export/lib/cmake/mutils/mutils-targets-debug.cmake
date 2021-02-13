#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "MUtils::MUtils" for configuration "Debug"
set_property(TARGET MUtils::MUtils APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(MUtils::MUtils PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C;CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/MUtils-debug.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS MUtils::MUtils )
list(APPEND _IMPORT_CHECK_FILES_FOR_MUtils::MUtils "${_IMPORT_PREFIX}/lib/MUtils-debug.lib" )

# Import target "MUtils::unittests" for configuration "Debug"
set_property(TARGET MUtils::unittests APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(MUtils::unittests PROPERTIES
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/unittests.exe"
  )

list(APPEND _IMPORT_CHECK_TARGETS MUtils::unittests )
list(APPEND _IMPORT_CHECK_FILES_FOR_MUtils::unittests "${_IMPORT_PREFIX}/bin/unittests.exe" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
