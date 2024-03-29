cmake_minimum_required(VERSION 3.12.4)
project(range_decompressor VERSION 1.0.0)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c*)
set(RANGECMP_SOURCES)
LIST(APPEND RANGECMP_SOURCES ${SOURCES})

if(MSVC)
	source_group("src" FILES ${RANGECMP_SOURCES})
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/examples/${PROJECT_NAME}>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/examples/${PROJECT_NAME}>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/examples/${PROJECT_NAME}>)

set(PROJECT_LINK_LIBS common novatel stream_interface)

link_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/decoders/common
	${CMAKE_CURRENT_SOURCE_DIR}/../../../bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/decoders/novatel
	${CMAKE_CURRENT_SOURCE_DIR}/../../../bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/hw_interface/stream_interface
	${CMAKE_CURRENT_SOURCE_DIR}/../../../bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/range_cmp
)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../../../extensions/rangecmp/api
	${CMAKE_CURRENT_SOURCE_DIR}/../../../src/decoders/common/api
	${CMAKE_CURRENT_SOURCE_DIR}/../../../src/decoders/novatel/api
	${CMAKE_CURRENT_SOURCE_DIR}/../../../src/hw_interface/stream_interface/api
	${CMAKE_CURRENT_SOURCE_DIR}/../../../src
)

add_executable(${PROJECT_NAME} range_decompressor_example.cpp)
add_dependencies(${PROJECT_NAME} common novatel stream_interface)
target_link_libraries(${PROJECT_NAME} ${PROJECT_LINK_LIBS})

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "examples")