cmake_minimum_required(VERSION 3.12.4)
project(SerialTestExample VERSION 1.0.0)

file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.c*)
set(SERIAL_TEST_SOURCES)
LIST(APPEND SERIAL_TEST_SOURCES ${SOURCES})

if(WINDOWS)
source_group("src" FILES ${SERIAL_TEST_SOURCES})
#set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "Example")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${PROJECT_NAME}/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}-${ARCH})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${PROJECT_NAME}/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}-${ARCH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${PROJECT_NAME}/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}-${ARCH})

set( PROJECT_LINK_LIBS Common.lib JsonInterface.lib Novatel.lib StreamInterface.lib DriverInterfaceAPI.lib mpr.lib Ws2_32.lib)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../bin/Common/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}-${ARCH})
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../bin/JsonInterface/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}-${ARCH})
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../bin/Novatel/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}-${ARCH})
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../bin/StreamInterface/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}-${ARCH})

if("${CMAKE_GENERATOR_PLATFORM}" STREQUAL Win32)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/driverinterface/bin/Win32)
else()
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/driverinterface/bin/x64)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/decoders/common/api)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/decoders/jsoninterface/api)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/decoders/novatel/api)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/hw_interface/stream_interface/api)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src/decoders/novatel/api/filters)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../lib/driverinterface/api)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src)

add_executable(serialtest main.cpp)
target_link_libraries(serialtest ${PROJECT_LINK_LIBS} )