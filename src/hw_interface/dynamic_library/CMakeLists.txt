cmake_minimum_required(VERSION 3.12.4)
project(hwinterface_dynamic_library VERSION 1.0.0)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c* ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h* ${CMAKE_CURRENT_SOURCE_DIR}/api/*.h*)
set(HWINTERFACE_DYNAMIC_LIBRARY_SOURCES)
LIST(APPEND HWINTERFACE_DYNAMIC_LIBRARY_SOURCES ${SOURCES})

if(MSVC)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HWINTERFACE_DYNAMIC_LIBRARY_SOURCES})
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/hw_interface/${PROJECT_NAME}>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/hw_interface/${PROJECT_NAME}>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/hw_interface/${PROJECT_NAME}>)

#Creating an object file which can be used for both static and shared build without recompilation.
add_library(objdynamichwinterface OBJECT ${HWINTERFACE_DYNAMIC_LIBRARY_SOURCES})

#Creating a shared/dynamic lib, with the project name, using available object files, without recompilation.
add_library(${PROJECT_NAME} SHARED $<TARGET_OBJECTS:objdynamichwinterface> $<TARGET_OBJECTS:objstreaminterface> $<TARGET_OBJECTS:objcommon>)

#Copy the api folder to the bin output directory so they can be easily used
add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/api/ ${CMAKE_SOURCE_DIR}/bin/api/hw_interface/${PROJECT_NAME}/api/)

set_property(TARGET ${PROJECT_NAME}       PROPERTY FOLDER "hw_interface")
set_property(TARGET objdynamichwinterface PROPERTY FOLDER "hw_interface")

if (WIN32)
    add_compile_definitions(HWINTERFACE_EXPORTS)
else()
    add_compile_definitions(HWINTERFACE_STATIC_DEFINE)
endif()

target_include_directories(
    objdynamichwinterface PUBLIC src
    objdynamichwinterface PUBLIC api
    objdynamichwinterface PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../
    objdynamichwinterface PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../
)
