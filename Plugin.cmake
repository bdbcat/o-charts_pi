# ~~~
# Author:      bdcat <Dave Register>
# Copyright:
# License:     GPLv2+
# ~~~

# -------- Cmake setup ---------
#

set(OCPN_TEST_REPO
    "david-register/ocpn-plugins-unstable"
    CACHE STRING "Default repository for untagged builds"
)
set(OCPN_BETA_REPO
    "opencpn/o-charts-beta"
    CACHE STRING
    "Default repository for tagged builds matching 'beta'"
)
set(OCPN_RELEASE_REPO
    "opencpn/o-charts-prod"
    CACHE STRING
    "Default repository for tagged builds not matching 'beta'"
)

set(OCPN_TARGET_TUPLE "" CACHE STRING
  "Target spec: \"platform;version;arch\""
)

# -------  Plugin setup --------
#
include("VERSION.cmake")
set(PKG_NAME o-charts_pi)
set(PKG_VERSION ${OCPN_VERSION})

set(PKG_PRERELEASE "")  # Empty, or a tag like 'beta'

set(DISPLAY_NAME o-charts)    # Dialogs, installer artifacts, ...
set(PLUGIN_API_NAME o-charts) # As of GetCommonName() in plugin API
set(CPACK_PACKAGE_CONTACT "Dave Register")
set(PKG_SUMMARY
  "Encrypted Vector Charts from o-charts.org"
)
set(PKG_DESCRIPTION [=[
OpenCPN  Vector Charts licensed and sourced from chart providers like
Hydrographic Offices.

While the charts are not officially approved official ENC charts they
are based on the same data -- the legal conditions are described in the
EULA. The charts has world-wide coverage and provides a cost-effective
way to access the national chart databases. Charts are encrypted and
can only be used after purchasing decryption keys from o-charts.org.

o-charts can handle all charts previously handled by the oesenc and
oernc plugins. It thus obsoletes these two plugins.
]=])

set(PKG_HOMEPAGE https://github.com/bdbcat/o-charts_pi)
set(PKG_INFO_URL https://o-charts.org/)

set(PKG_AUTHOR "Dave register")

if(NOT QT_ANDROID AND NOT APPLE)
  set(OCPN_BUILD_USE_GLEW ON)
else(NOT QT_ANDROID AND NOT APPLE)
  set(OCPN_BUILD_USE_GLEW OFF)
endif(NOT QT_ANDROID AND NOT APPLE)

if(OCPN_BUILD_USE_GLEW)
  add_definitions(-D__OCPN_USE_GLEW__)
  include_directories("/app/extensions/o-charts_pi/include")

  if (CMAKE_HOST_WIN32)
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/buildwin/include/glew")
    link_libraries(${CMAKE_SOURCE_DIR}/buildwin/glew32.lib )
  endif (CMAKE_HOST_WIN32)

endif(OCPN_BUILD_USE_GLEW)

if (CMAKE_HOST_WIN32)
  add_definitions(-D__MSVC__)
endif (CMAKE_HOST_WIN32)


set(SRC
  src/chart.cpp
  src/cutil.cpp
  src/eSENCChart.cpp
  src/eSENCChart.h
  src/fpr.cpp
  src/georef.cpp
  src/InstallDirs.cpp
  src/InstallDirs.h
  src/ochartShop.cpp
  src/ochartShop.h
  src/o-charts_pi.cpp
  src/o-charts_pi.h
  src/OCPNRegion.cpp
  src/oernc_inStream.cpp
  src/Osenc.cpp
  src/Osenc.h
  src/piScreenLog.cpp
  src/s57RegistrarMgr.cpp
  src/sha256.c
  src/uKey.cpp
  src/validator.cpp
  src/viewport.cpp
  libs/gdal/src/s57classregistrar.cpp
)

if(QT_ANDROID)
  set(SRC ${SRC} src/androidSupport.cpp)
endif(QT_ANDROID)

if(NOT QT_ANDROID)
add_compile_definitions( ocpnUSE_GLSL )
add_compile_definitions( ocpnUSE_GL )
endif(NOT QT_ANDROID)

if(QT_ANDROID)
  add_definitions(-D__OCPN__ANDROID__)
endif(QT_ANDROID)

set(PKG_API_LIB api-17)  #  A directory in libs/ e. g., api-17 or api-16

macro(late_init)
  # Perform initialization after the PACKAGE_NAME library, compilers
  # and ocpn::api is available.
  target_include_directories(${PACKAGE_NAME} PRIVATE libs/gdal/src src)

  # A specific target requires special handling
  # When OCPN core is a ubuntu-bionic build, and the debian-10 plugin is loaded,
  #   then there is conflict between wxCurl in core, vs plugin
  # So, in this case, disable the plugin's use of wxCurl directly, and revert to
  #   plugin API for network access.
  # And, Android always uses plugin API for network access
  string(TOLOWER "${OCPN_TARGET_TUPLE}" _lc_target)
  message(STATUS "late_init: ${OCPN_TARGET_TUPLE}.")

  if ( (NOT "${_lc_target}" MATCHES "debian;10;x86_64") AND
       (NOT "${_lc_target}" MATCHES "android*") )
    add_definitions(-D__OCPN_USE_CURL__)
  endif()

endmacro ()

macro(add_plugin_libraries)
  add_subdirectory("libs/cpl")
  target_link_libraries(${PACKAGE_NAME} ocpn::cpl)

  add_subdirectory("libs/dsa")
  target_link_libraries(${PACKAGE_NAME} ocpn::dsa)

  add_subdirectory("libs/wxJSON")
  target_link_libraries(${PACKAGE_NAME} ocpn::wxjson)

  add_subdirectory("libs/iso8211")
  target_link_libraries(${PACKAGE_NAME} ocpn::iso8211)

  add_subdirectory("libs/tinyxml")
  target_link_libraries(${PACKAGE_NAME} ocpn::tinyxml)

  add_subdirectory("libs/zlib")
  target_link_libraries(${PACKAGE_NAME} ocpn::zlib)

  add_subdirectory(libs/s52plib)
  target_link_libraries(${PACKAGE_NAME} ocpn::s52plib)

  add_subdirectory(libs/geoprim)
  target_link_libraries(${PACKAGE_NAME} ocpn::geoprim)

  add_subdirectory(libs/pugixml)
  target_link_libraries(${PACKAGE_NAME} ocpn::pugixml)

  add_subdirectory("libs/wxcurl")
  target_link_libraries(${PACKAGE_NAME} ocpn::wxcurl)

# For OCPN 5.6.2, which does not use GLES in core, we need to add it here.
if (NOT OCPN_NOGLEW)
  if (UNIX AND NOT APPLE AND NOT QT_ANDROID)   # linux
    if(OCPN_BUILD_USE_GLEW)
      target_link_libraries(${PACKAGE_NAME} "GLEW")
    endif(OCPN_BUILD_USE_GLEW)
  endif(UNIX AND NOT APPLE)
endif (NOT OCPN_NOGLEW)

  add_subdirectory("libs/oeserverd")

  if(QT_ANDROID)
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/includeAndroid")
  endif(QT_ANDROID)

endmacro ()
