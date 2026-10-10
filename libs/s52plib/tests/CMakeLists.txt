cmake_minimum_required(VERSION 3.12)
project(raster_symbol_tests LANGUAGES CXX)

# Standalone regression tests: cmake -S libs/s52plib/tests -B build-symbol-tests
# This leaves the plugin's production build and API dependencies unchanged.
find_package(wxWidgets REQUIRED COMPONENTS core base)
include(${wxWidgets_USE_FILE})
add_executable(raster_symbol_tests raster_symbol_tests.cpp)
target_compile_features(raster_symbol_tests PRIVATE cxx_std_11)
target_link_libraries(raster_symbol_tests PRIVATE ${wxWidgets_LIBRARIES})
enable_testing()
add_test(NAME raster_symbol_regressions COMMAND raster_symbol_tests)
