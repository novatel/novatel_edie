cmake_minimum_required(VERSION 3.10.2)

project(JsonInterfaceTest VERSION 1.0.0)

file (GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
set(JSONINTEFACETEST_SOURCES)
LIST(APPEND JSONINTEFACETEST_SOURCES ${SOURCES})

add_executable(${PROJECT_NAME} ${JSONINTEFACETEST_SOURCES})

add_test(${PROJECT_NAME} COMMAND ${PROJECT_NAME})
add_definitions(-DDATADIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "decoders/tests")

get_directory_property( DirDefs COMPILE_DEFINITIONS )
message( "COMPILE_DEFINITIONS = ${DirDefs}" )
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../)
target_link_libraries(${PROJECT_NAME} PUBLIC Common Novatel JsonInterface gtest -fprofile-arcs)
