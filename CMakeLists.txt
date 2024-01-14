# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.14)
project(frame)
enable_testing()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    crc
    GIT_REPOSITORY https://github.com/kodebooth/crc.git
    GIT_TAG 3a56660332fce885de6d2804a502d8f84bd4dc29
)
FetchContent_MakeAvailable(crc)

add_library(frame INTERFACE)
target_include_directories(frame INTERFACE include)
target_link_libraries(frame INTERFACE crc)

include(gtest.cmake)
add_subdirectory(test)