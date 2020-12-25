# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("VLCApp")

if(MSVC)
add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else(MSVC)
endif(MSVC)
# Include sub-projects.
add_subdirectory ("vendor/zlib-1.2.11")
add_subdirectory ("vendor/cares-1.16.0")
add_subdirectory ("vendor/libiconv-1.16")
add_subdirectory ("vendor/libressl-3.1.1")
add_subdirectory ("vendor/libssh2-1.9.0")
add_subdirectory ("vendor/libcurl-7.69.1")
add_subdirectory ("VLCApp")
