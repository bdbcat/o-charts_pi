
o-charts_pi bundled glew
------------------------

The bundled 2.2.0 glew2 sources are pristine besides the following
patch which is required to avoid FTBFS on recent cmake versions.
The problem is known on cmake 4.0.2, but might affect also 
earlier versions

---


diff --git a/libs/glew/glew-2.2.0/build/cmake/CMakeLists.txt b/libs/glew/glew-2.2.0/build/cmake/CMakeLists.txt
index 419c2435..50bba363 100644
--- a/libs/glew/glew-2.2.0/build/cmake/CMakeLists.txt
+++ b/libs/glew/glew-2.2.0/build/cmake/CMakeLists.txt
@@ -4,7 +4,7 @@ endif ()
 
 project (glew C)
 
-cmake_minimum_required (VERSION 2.8.12)
+cmake_minimum_required (VERSION 3.5.10)
 
 include(GNUInstallDirs)
