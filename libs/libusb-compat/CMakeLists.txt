# ~~~
# Summary:      Build libusb-0.1 compat library
# License:      Expat
# Copyright (c) 2026 Alec Leamas
#
# Exports:      libusb::compat transitive link target
# ~~~

if (TARGET libusb::compat)
  return()
endif ()

cmake_minimum_required(VERSION 3.11)
project("libusb-compat")

set(repo "https://github.com/libusb/libusb-compat-0.1.git")
set(tag v0.1.9)

add_library(_LIBUSB_IF INTERFACE)
add_library(libusb::compat ALIAS _LIBUSB_IF)

if(CMAKE_VERSION VERSION_LESS 3.25 AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(LINUX true)
endif()

if (NOT LINUX)
  message(STATUS "Using empty libusb::compat on non-Linux platforms")
  return()
endif ()

if (${BUILD_TYPE} STREQUAL "flatpak" OR DEFINED ENV{FLATPAK_ID})
  message(STATUS "Ignoring libusb-compat build on Flatpak where it is a module")
  return ()
endif ()

include(ExternalProject)

set(_LIBUSB_LIBS
  ${CMAKE_BINARY_DIR}/libusb-install/lib/libusb-0.1.so.4
  ${CMAKE_BINARY_DIR}/libusb-install/lib/libusb-0.1.so.4.4.4
  ${CMAKE_BINARY_DIR}/libusb-install/lib/libusb-0.1.so
)
ExternalProject_Add(_libusb_compat
  GIT_REPOSITORY ${repo}
  GIT_TAG ${tag}
  SOURCE_DIR ${CMAKE_BINARY_DIR}/libusb-compat
  BUILD_IN_SOURCE TRUE
  INSTALL_DIR ${CMAKE_BINARY_DIR}/libusb-install
  PATCH_COMMAND ./bootstrap.sh
  CONFIGURE_COMMAND ./configure --disable-static --enable-shared --prefix=/
  BUILD_COMMAND make
  INSTALL_COMMAND make DESTDIR=${CMAKE_BINARY_DIR}/libusb-install install
)
add_custom_target(
  _libusb_compat_build
  COMMAND ${CMAKE_COMMAND} -E echo "Built the libusb-compat library"
  COMMAND ${CMAKE_COMMAND} -E rename libusb.so libusb-0.1.so
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/libusb-install/lib
  DEPENDS _libusb_compat
  BYPRODUCTS ${_LIBUSB_LIBS}
)
add_dependencies(_LIBUSB_IF _libusb_compat_build)
target_link_libraries(_LIBUSB_IF INTERFACE ${_LIBUSB_LIBS})

install(
  FILES ${_LIBUSB_LIBS} DESTINATION lib/opencpn
  PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ
)
