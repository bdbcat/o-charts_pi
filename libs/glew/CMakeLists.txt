#
# Build the static libglew library
#
# Exports: glew2:glew2 transitive link target
#

# Copyright (C) 2025 Alec Leamas
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.

cmake_minimum_required(VERSION 3.10)

if (TARGET glew2::glew2)
  return ()
endif ()

add_library(_glew_if INTERFACE)
add_library(glew2::glew2 ALIAS _glew_if)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/glew-2.2.0/build/cmake)
target_link_libraries(_glew_if INTERFACE glew_s)
