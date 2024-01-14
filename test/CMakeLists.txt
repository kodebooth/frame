#SPDX - License - Identifier : MIT

cmake_minimum_required(VERSION 3.14)
enable_testing()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(
    frame_test
    encoder_test.cpp
    decoder_test.cpp
    integrity_check_test.cpp
)

target_link_libraries(
    frame_test
    frame
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(frame_test)