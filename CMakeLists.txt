cmake_minimum_required(VERSION 3.6)

project(slots)

add_subdirectory(magidoor)
include_directories(${CMAKE_SOURCE_DIR}/magidoor)
add_executable(slots main.c)
if (WIN32)
	TARGET_LINK_LIBRARIES(slots mdoor Ws2_32)
Else()
	TARGET_LINK_LIBRARIES(slots mdoor)
endif()
