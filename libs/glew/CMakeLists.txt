# ~~~
# For platforms requiring, build and link the glew library
#
# Exports: glew2::glew2 transitive link target
#
# License:      GPLv3+
# Copyright (c) 2025 Alec Leamas
# ~~~

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

cmake_minimum_required(VERSION 3.10.0)

if (TARGET glew1::glew2)
  return ()
endif ()

set(
  src_url https://sourceforge.net/projects/glew/files/glew/2.2.0/glew-2.2.0.tgz
)
set(src_hash MD5=3579164bccaef09e36c0af7f4fd5c7c7)

list(APPEND patches
  0001-cmake-Update-ancient-required-version.patch
  0002-build-cmake-windows-Don-t-install-pdb-file-on-static.patch
)

if (POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif ()

list(PREPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

add_library(_GLEW_IF INTERFACE)
add_library(glew2::glew2 ALIAS _GLEW_IF)

if (QT_ANDROID OR DEFINED ENV{FLATPAK_ID})
  # glew does not build on Android due to missing glx and others.
  # OTOH it's not needed there. For Flatpak, glew is supposed to
  # be installed in standard directories as a depency
  # Return an empty link target in those cases
  message(STATUS "glew: using empty target glew2::glew2")
  return ()
endif ()

# Download sources from github
#
if (${CMAKE_VERSION} VERSION_LESS 3.20)
  message(WARNING "Using downloaded sources requires cmake >= 3.20")
endif ()

if (POLICY CMP0169)
  # FetchContent_Populate cannot be used if NEW
  cmake_policy(SET CMP0169 OLD)
endif ()

include(FetchContent)
FetchContent_Declare(glew_src URL ${src_url} URL_HASH ${src_hash})
FetchContent_Populate(glew_src)
FetchContent_GetProperties(glew_src SOURCE_DIR src_dir)
message(STATUS "Using downloaded glew at ${src_dir}")

# Apply patches
#
foreach (patch ${patches})
  execute_process(
    COMMAND ${CMAKE_COMMAND}
      -Dpatch_file=${CMAKE_CURRENT_SOURCE_DIR}/patches/${patch}
      -Dpatch_dir=${src_dir}
      -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/PatchFile.cmake
    COMMAND ${CMAKE_COMMAND} -E echo "-- Applying patch ${patch}"
  )
endforeach ()

# Configure
#
set(BUILD_UTILS OFF CACHE BOOL "")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
add_subdirectory(${src_dir}/build/cmake glew_dir)

target_link_libraries(_GLEW_IF INTERFACE glew_s)
target_include_directories(_GLEW_IF INTERFACE "${src_dir}/include")
