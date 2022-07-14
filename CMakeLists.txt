cmake_minimum_required(VERSION 3.22)
set (CMAKE_CXX_STANDARD 17)
project(i2p_sam)

# add the executable
add_executable(i2p_sam main.cpp "i2p_sam.h" "i2p_sam.cpp")

if ((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
message("Build with Clang")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wextra -Werror")
elseif ((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
message("Build with GNU")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lboost_system -lboost_thread -lpthread -Wall -Wextra -Werror")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC") 
message("Build with MSVC")
set(CMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
else()
message([FATAL_ERROR] "Compiler not supported")
endif()

find_package(Boost COMPONENTS system)
include_directories(SYSTEM ${Boost_INCLUDE_DIRS})