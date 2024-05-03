cmake_minimum_required(VERSION 3.15)
project(test_package CXX)

find_package(novatel_edie REQUIRED CONFIG)

add_executable(CommandEncoding ${CMAKE_CURRENT_LIST_DIR}/../examples/novatel/command_encoding/command_encoding.cpp)
target_link_libraries(CommandEncoding novatel_edie::novatel_edie)
target_compile_features(CommandEncoding PRIVATE cxx_std_20)
