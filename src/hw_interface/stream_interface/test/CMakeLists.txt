cmake_minimum_required(VERSION 3.10.2)

project(StreamInterfaceTest VERSION 1.0.0)

file (GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
set(STREAMINTERFACETEST_SOURCES)
LIST(APPEND STREAMINTERFACETEST_SOURCES ${SOURCES})

add_executable(${PROJECT_NAME} ${STREAMINTERFACETEST_SOURCES})
add_definitions(-DDATADIR="${CMAKE_CURRENT_SOURCE_DIR}/resources/")
add_test(${PROJECT_NAME} COMMAND ${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "hw_interface/tests")

get_directory_property( DirDefs COMPILE_DEFINITIONS )
message( "COMPILE_DEFINITIONS = ${DirDefs}" )
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../decoders/common/api)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../decoders/novatel/api)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../decoders/novatel/api/filters)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../decoders/jsoninterface/api)
target_link_libraries(${PROJECT_NAME} PUBLIC Novatel Common StreamInterface JsonInterface gtest -fprofile-arcs)

