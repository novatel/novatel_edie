cmake_minimum_required(VERSION 3.12.4)

project(StreamInterfaceTest VERSION 1.0.0)

file (GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c* GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.h*)
set(STREAMINTERFACETEST_SOURCES)
LIST(APPEND STREAMINTERFACETEST_SOURCES ${SOURCES})

add_executable(${PROJECT_NAME} ${STREAMINTERFACETEST_SOURCES})
add_test(${PROJECT_NAME} COMMAND ${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "hw_interface/tests")

get_directory_property(DirDefs COMPILE_DEFINITIONS)
target_include_directories(
	${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../
	${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../decoders/common/api
	${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../decoders/novatel/api
	${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../decoders/novatel/api/filters
)
target_link_libraries(${PROJECT_NAME} PUBLIC novatel common stream_interface gtest)

