cmake_minimum_required(VERSION 3.12.4)
project(stream_interface VERSION 1.0.0)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c* ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h* ${CMAKE_CURRENT_SOURCE_DIR}/api/*.h*)
set(STREAM_SOURCES)
LIST(APPEND STREAM_SOURCES ${SOURCES})

if(LINUX)
    LIST(REMOVE_ITEM STREAM_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/inputportstream.cpp)
    LIST(REMOVE_ITEM STREAM_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/outputportstream.cpp)
endif()

if(WINDOWS)
    #source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NOVATEL_SOURCES})
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/hw_interface/${PROJECT_NAME}>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/hw_interface/${PROJECT_NAME}>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/hw_interface/${PROJECT_NAME}>)

#Creating an object file which can be used for both static and shared build without recompilation.
add_library(objstreaminterface OBJECT ${STREAM_SOURCES})

#Creating a static lib, with the project name, using available object file, without recompilation.
add_library(${PROJECT_NAME} STATIC $<TARGET_OBJECTS:objstreaminterface> $<TARGET_OBJECTS:objcommon>)

#Copy the api folder to the bin output directory so they can be easily used
add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/api/ ${CMAKE_SOURCE_DIR}/bin/api/hw_interface/${PROJECT_NAME}/api/)

set_property(TARGET ${PROJECT_NAME}    PROPERTY FOLDER "hw_interface")
set_property(TARGET objstreaminterface PROPERTY FOLDER "hw_interface")

target_include_directories(
    objstreaminterface PUBLIC src
    objstreaminterface PUBLIC api
    objstreaminterface PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../
    objstreaminterface PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../
    objstreaminterface PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib/driverinterface/api
)

if(LINUX)
    file(GLOB_RECURSE MY_PUBLIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/api/*.h*)
    set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${MY_PUBLIC_HEADERS}")
    
    #Install the shared/dynamic lib.
    if(CMAKE_LIB_SHARED)
        install(TARGETS ${PROJECT_NAME} DynamicStreamInterface
            ARCHIVE DESTINATION /usr/lib
            LIBRARY DESTINATION /usr/lib
            PUBLIC_HEADER DESTINATION /usr/include/novatel/edie/hw_interface/stream_interface)
    else()
        install(TARGETS ${PROJECT_NAME}
            ARCHIVE DESTINATION /usr/lib
            LIBRARY DESTINATION /usr/lib
            PUBLIC_HEADER DESTINATION /usr/include/novatel/edie/hw_interface/stream_interface)
    endif()
endif()
