cmake_minimum_required(3.9.1)

project(Wayland-Display)

set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall --save-temps=obj")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
include_directories(${CMAKE_SOURCE_DIR}/.)
message(STATUS, "include directory ${CMAKE_SOURCE_DIR}")

file(GLOB SOURCES "src/*.cpp")

target_link_libraries(wayland-client)
add_executable(wayland_display ${SOURCES});
