cmake_minimum_required(VERSION 3.12.4)
project(common VERSION 1.0.0)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c* ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h* ${CMAKE_CURRENT_SOURCE_DIR}/api/*.h*)
set(COMMON_SOURCES)
LIST(APPEND COMMON_SOURCES ${SOURCES})

if(MSVC)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NOVATEL_SOURCES})
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/decoders/${PROJECT_NAME}>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/decoders/${PROJECT_NAME}>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/decoders/${PROJECT_NAME}>)

# Creating an object file which can be used for both static and shared build without recompilation.
add_library(objcommon OBJECT ${COMMON_SOURCES})

# Creating a static lib, with the project name, using available object file, without recompilation.
add_library(${PROJECT_NAME} STATIC $<TARGET_OBJECTS:objcommon>)

# Copy the api folder to the bin output directory so they can be easily used
add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/api/ ${CMAKE_SOURCE_DIR}/bin/api/decoders/${PROJECT_NAME}/api/)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "decoders")
set_property(TARGET objcommon       PROPERTY FOLDER "decoders")

target_include_directories(
    objcommon PUBLIC src
    objcommon PUBLIC api
    objcommon PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../
)

if(LINUX)
    file(GLOB_RECURSE MY_PUBLIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/api/*.h*)
    set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${MY_PUBLIC_HEADERS}")

    #Install the shared/dynamic lib.
    if(CMAKE_LIB_SHARED)
        install(TARGETS ${PROJECT_NAME} DynamicCommon
        ARCHIVE DESTINATION /usr/lib
        LIBRARY DESTINATION /usr/lib
        PUBLIC_HEADER DESTINATION /usr/include/novatel/edie/decoders/common)
    else()
        install(TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION /usr/lib
        LIBRARY DESTINATION /usr/lib
        PUBLIC_HEADER DESTINATION /usr/include/novatel/edie/decoders/common)
    endif()
endif()
