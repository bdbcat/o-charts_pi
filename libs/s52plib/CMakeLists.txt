cmake_minimum_required(VERSION 3.5)

if (TARGET ocpn::s52plib)
    return ()
endif ()

if (NOT CMAKE_MODULE_PATH)
  set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../cmake)
endif ()

SET(SRC
    src/chartsymbols.cpp
    src/s52plib.cpp
    src/s52cnsy.cpp
    src/TexFont.cpp
    src/s52utils.cpp
    src/s52shaders.cpp
    src/mygeomOC.cpp
    src/DepthFont.cpp
    src/Cs52_shaders.cpp
    )

if (NOT wxWidgets_INCLUDE_DIRS)
    include(SimpleWxConfig)
    SimpleWxConfig()
endif ()

add_library(S52PLIB STATIC ${SRC})
add_library(ocpn::s52plib ALIAS S52PLIB)

set_property(TARGET S52PLIB PROPERTY COMPILE_FLAGS "${OBJ_VISIBILITY}")
target_include_directories(S52PLIB PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(S52PLIB PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_include_directories(S52PLIB PRIVATE ../geoprim/src)
target_include_directories(S52PLIB PRIVATE ../pugixml)
target_include_directories(S52PLIB PRIVATE ../../include)
target_include_directories(S52PLIB PRIVATE ../../opencpn-libs/api-17)
target_link_libraries(S52PLIB PRIVATE glew2::glew2)

if(QT_ANDROID)
    target_include_directories(S52PLIB PRIVATE ../../includeAndroid)
endif(QT_ANDROID)

