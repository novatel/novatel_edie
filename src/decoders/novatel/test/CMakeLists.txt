cmake_minimum_required(VERSION 3.12.4)

project(NovatelTest VERSION 1.0.0)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c* ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)
set(NOVATELTEST_SOURCES)
LIST(APPEND NOVATELTEST_SOURCES ${SOURCES})

add_executable(${PROJECT_NAME} ${NOVATELTEST_SOURCES})

add_test(${PROJECT_NAME} COMMAND ${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "decoders/tests")

include_directories(${CMAKE_SOURCE_DIR}/src/decoders/common/api)

get_directory_property( DirDefs COMPILE_DEFINITIONS )
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../)
target_link_libraries(${PROJECT_NAME} PUBLIC novatel common stream_interface gtest)
