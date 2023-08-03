cmake_minimum_required(VERSION 3.12.4)

project(CommonTest VERSION 1.0.0)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
set(COMMONTEST_SOURCES)
LIST(APPEND COMMONTEST_SOURCES ${SOURCES})

add_executable(${PROJECT_NAME} ${COMMONTEST_SOURCES})

add_test(${PROJECT_NAME} COMMAND ${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "decoders/tests")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../api)

get_directory_property( DirDefs COMPILE_DEFINITIONS )
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../)

target_link_libraries(${PROJECT_NAME} PUBLIC common gtest)
