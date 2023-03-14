cmake_minimum_required(VERSION 3.10.2)

project(NovatelTest VERSION 1.0.0)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c*)
set(NOVATELTEST_SOURCES)
LIST(APPEND NOVATELTEST_SOURCES ${SOURCES})

add_executable(${PROJECT_NAME} ${NOVATELTEST_SOURCES})

add_test(${PROJECT_NAME} COMMAND ${PROJECT_NAME})
add_definitions(-DDATADIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "decoders/tests")

get_directory_property( DirDefs COMPILE_DEFINITIONS )
message( "COMPILE_DEFINITIONS = ${DirDefs}" )
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../)
target_link_libraries(${PROJECT_NAME} PUBLIC Novatel Common JsonInterface StreamInterface gtest -fprofile-arcs)
