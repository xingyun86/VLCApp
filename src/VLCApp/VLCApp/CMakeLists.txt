# CMakeList.txt : CMake project for VLCApp, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(project_name "VLCApp")
project(${project_name})

if(MSVC)
	include_directories(
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_SOURCE_DIR}/vendor/utils
		${CMAKE_SOURCE_DIR}/vendor/libvlc/3.0.11-win64/sdk/include
		${CMAKE_SOURCE_DIR}/vendor/libcurl-7.69.1/include
		${CMAKE_SOURCE_DIR}/vendor/libiconv-1.16/include
		${CMAKE_BINARY_DIR}/vendor/libiconv-1.16
	)
	link_directories(
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_SOURCE_DIR}/vendor/libvlc/3.0.11-win64/sdk/lib
		${CMAKE_BINARY_DIR}/vendor/libcurl-7.69.1/lib
		${CMAKE_BINARY_DIR}/vendor/cares-1.16.0/lib
		${CMAKE_BINARY_DIR}/vendor/libiconv-1.16
	)
	set(EXT_LIB libvlc libvlccore libcurl_static iconv_static)
else(MSVC)
	include_directories(
		${CMAKE_CURRENT_SOURCE_DIR}
	)
	link_directories(
		${CMAKE_CURRENT_BINARY_DIR}
	)
	set(EXT_LIB vlc vlccore)
endif(MSVC)

file(GLOB SRCS "*.c" "*.cpp")

# Add source to this project's executable.
add_executable (${project_name} ${SRCS})

if(MSVC)
	target_compile_options(${project_name} PRIVATE "/std:c++latest")
else(MSVC)
endif(MSVC)
target_link_libraries(${project_name} PRIVATE ${EXT_LIB})
# TODO: Add tests and install targets if needed.
