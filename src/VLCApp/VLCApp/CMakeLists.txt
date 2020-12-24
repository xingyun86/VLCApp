# CMakeList.txt : CMake project for VLCApp, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

set(project_name "VLCApp")
project(${project_name})

if(MSVC)
	include_directories(
		${CMAKE_CURRENT_SOURCE_DIR}
		${CMAKE_SOURCE_DIR}/vendor/libvlc/3.0.11-win64/sdk/include
	)
	link_directories(
		${CMAKE_CURRENT_BINARY_DIR}
		${CMAKE_SOURCE_DIR}/vendor/libvlc/3.0.11-win64/sdk/lib
	)
	set(EXT_LIB libvlc libvlccore)
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

target_link_libraries(${project_name} PRIVATE ${EXT_LIB})
# TODO: Add tests and install targets if needed.
