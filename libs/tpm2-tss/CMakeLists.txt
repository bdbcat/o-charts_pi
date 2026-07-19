# ~~~
# Summary:      Build tpm2-tss libtss2-esys library and deps
# License:      Expat
# Copyright (c) 2026 Alec Leamas
#
# Exports:      tpm2-tss::tpm2-tss transitive link target
# ~~~

if (TARGET tpm2-tss::tpm2-tss)
  return()
endif ()

cmake_minimum_required(VERSION 3.11)
project("tss2-esys")

set(tpm2_repo "https://github.com/tpm2-software/tpm2-tss.git")
set(tpm2_tag "4.1.3")

set(mnl_repo "https://github.com/justmirror/libmnl.git")
set(mnl_tag "libmnl-1.0.4")

set(json_c_repo "https://github.com/json-c/json-c")
set(json_c_tag "json-c-0.19-20260627")

add_library(_TPM2_IF INTERFACE)
add_library(tpm2-tss::tpm2-tss ALIAS _TPM2_IF)

if(CMAKE_VERSION VERSION_LESS 3.25 AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(LINUX true)
endif()

if (NOT LINUX)
  message(STATUS "Using empty tpm2-tss::tpm2-tss on non-Linux platforms")
  return()
endif ()

if (${BUILD_TYPE} STREQUAL "flatpak" OR DEFINED ENV{FLATPAK_ID})
  message(STATUS "Ignoring tpm2-css build on Flatpak where it is a module")
  return ()
endif ()

include(ExternalProject)

set(_MNL_LIBS
  ${CMAKE_BINARY_DIR}/libmnl-install/lib/libmnl.so.0.2.0
  ${CMAKE_BINARY_DIR}/libmnl-install/lib/libmnl.so
)
ExternalProject_Add(_mnl
  GIT_REPOSITORY ${mnl_repo}
  GIT_TAG ${mnl_tag}
  SOURCE_DIR ${CMAKE_BINARY_DIR}/libmnl
  BUILD_IN_SOURCE TRUE
  INSTALL_DIR ${CMAKE_BINARY_DIR}/libmnl-install
  PATCH_COMMAND ./autogen.sh
  CONFIGURE_COMMAND ./configure --disable-static --enable-shared --prefix=/
  BUILD_COMMAND make
  INSTALL_COMMAND make DESTDIR=${CMAKE_BINARY_DIR}/libmnl-install install
)
add_custom_target(
  _mnl_build
  COMMAND  ${CMAKE_COMMAND} -E echo "Built the libmnl library"
  DEPENDS _mnl
  BYPRODUCTS ${_MNL_LIBS}
)

set(_JSONC_LIBS
  ${CMAKE_BINARY_DIR}/json-c-install/lib/libjson-c.so.5.5.0
  ${CMAKE_BINARY_DIR}/json-c-install/lib/libjson-c.so.5
  ${CMAKE_BINARY_DIR}/json-c-install/lib/libjson-c.so
)
ExternalProject_Add(_json_c
  GIT_REPOSITORY ${json_c_repo}
  GIT_TAG ${json_c_tag}
  PREFIX ${CMAKE_BINARY_DIR}/json-c
  CMAKE_ARGS
    -DCMAKE_INSTALL_LIBDIR=lib
    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/json-c-install
)
add_custom_target(
  _json_c_build
  COMMAND  ${CMAKE_COMMAND} -E echo "Built the json-c library"
  DEPENDS _json_c
  BYPRODUCTS ${_JSONC_LIBS}
)

set(_TPM2_LIBS
  ${CMAKE_BINARY_DIR}/tpm2-tss-install/lib/libtss2-esys.so.0.0.1
  ${CMAKE_BINARY_DIR}/tpm2-tss-install/lib/libtss2-esys.so
)
ExternalProject_Add(_tpm2
  GIT_REPOSITORY ${tpm2_repo}
  GIT_TAG ${tpm2_tag}
  SOURCE_DIR ${CMAKE_BINARY_DIR}/tpm2-tss
  BUILD_IN_SOURCE TRUE
  INSTALL_DIR ${CMAKE_BINARY_DIR}/tpm2-tss-install
  PATCH_COMMAND ./bootstrap
  LOG_CONFIGURE TRUE
  CONFIGURE_COMMAND
    cmake -E env
      "CFLAGS=-Wno-calloc-transposed-args -Wno-discarded-qualifiers"
      "JSONC_LIBS=-L${CMAKE_BINARY_DIR}/json-c-install/lib -ljson-c"
      "JSONC_CFLAGS=-I${CMAKE_BINARY_DIR}/json-c-install/include"
      ./configure
        --disable-static
        --enable-shared
        --without-udevrulesdir
        --with-tctidefaultconfig=/dev/tpmrm0
        --with-tctidefaultmodule=libtss2-tcti-device.so
        --disable-doxygen-doc
        --prefix=${CMAKE_BINARY_DIR}/tpm2-tss-install
  BUILD_COMMAND make
  INSTALL_COMMAND make install
)
add_custom_target(
  _tpm2_build
  COMMAND  ${CMAKE_COMMAND} -E echo "Built the tpm2-tss library"
  DEPENDS _tpm2
  BYPRODUCTS ${_TPM2_LIBS}
)

add_dependencies(_TPM2_IF _tpm2_build _mnl_build _json_c_build)
target_include_directories( _TPM2_IF INTERFACE
  ${CMAKE_BINARY_DIR}/tpm2-tss-install/include
  ${CMAKE_BINARY_DIR}/libmnl-install/include
)
target_link_libraries(_TPM2_IF INTERFACE  ${_TPM2_LIBS})

install(
  FILES ${_TPM2_LIBS} ${_MNL_LIBS} ${_JSONC_LIBS} DESTINATION lib/opencpn
  PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ
)
install(
  DIRECTORY ${CMAKE_BINARY_DIR}/tpm2-tss-install/include
  DESTINATION share/opencpn/plugins/${PACKAGE_NAME}
)
