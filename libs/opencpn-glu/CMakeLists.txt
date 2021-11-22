#
# On platforms without native libglu like Flatpak and Android:
# Bundle the GL/glu.h header which reflects the libglu available
# in main OpenCPN

cmake_minimum_required(VERSION 3.1)

if (TARGET opencpn::glu)
    return ()
endif ()

add_library(_glu INTERFACE)
add_library(opencpn::glu ALIAS _glu)

if (DEFINED ENV{FLATPAK_ID})
  target_include_directories(_glu INTERFACE include-flatpak)
elseif (QT_ANDROID)
  target_include_directories(_glu INTERFACE include-android)
else ()
  message(STATUS "Not using bundled GL/glu.h header")
endif ()
