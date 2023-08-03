cmake_minimum_required(VERSION 3.12.4)
project(NovAtelDynamic_FileParser VERSION 1.0.0)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c*)
set(NOVATEL_SOURCES)
LIST(APPEND NOVATEL_SOURCES ${SOURCES})

if(MSVC)
	source_group("src" FILES ${NOVATEL_SOURCES})
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}>)

set(PROJECT_LINK_LIBS common decoders_dynamic_library novatel stream_interface)

link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/decoders/common
	${CMAKE_CURRENT_SOURCE_DIR}/../../../bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/decoders/decoders_dynamic_library
	${CMAKE_CURRENT_SOURCE_DIR}/../../../bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/decoders/novatel
	${CMAKE_CURRENT_SOURCE_DIR}/../../../bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/hw_interface/stream_interface
)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../src/decoders/common/api
	${CMAKE_CURRENT_SOURCE_DIR}/../../../src/decoders/dynamic_library/api
	${CMAKE_CURRENT_SOURCE_DIR}/../../../src/decoders/novatel/api
	${CMAKE_CURRENT_SOURCE_DIR}/../../../src/hw_interface/stream_interface/api
	${CMAKE_CURRENT_SOURCE_DIR}/../../../src
)

add_executable(${PROJECT_NAME} dynamic_fileparser.cpp)
add_dependencies(${PROJECT_NAME} common novatel decoders_dynamic_library stream_interface)
target_link_libraries(${PROJECT_NAME} ${PROJECT_LINK_LIBS} )

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "examples")