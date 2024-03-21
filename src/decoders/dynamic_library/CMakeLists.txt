cmake_minimum_required(VERSION 3.12.4)
project(decoders_dynamic_library VERSION 1.0.0)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c* ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h* ${CMAKE_CURRENT_SOURCE_DIR}/api/*.h*)
set(DECODERS_DYNAMIC_LIBRARY_SOURCES)
LIST(APPEND DECODERS_DYNAMIC_LIBRARY_SOURCES ${SOURCES})

if(MSVC)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${DECODERS_DYNAMIC_LIBRARY_SOURCES})
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/decoders/${PROJECT_NAME}>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/decoders/${PROJECT_NAME}>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}-${ARCH}-${DISTRIB_NAME}/decoders/${PROJECT_NAME}>)

# Creating an object file which can be used for both static and shared build without recompilation.
 add_library(objdynamicdecoder OBJECT ${DECODERS_DYNAMIC_LIBRARY_SOURCES})

# Creating a shared/dynamic lib, with the project name, using available object files, without recompilation.
add_library(${PROJECT_NAME} SHARED $<TARGET_OBJECTS:objdynamicdecoder> $<TARGET_OBJECTS:objcommon> $<TARGET_OBJECTS:objnovatel> $<TARGET_OBJECTS:objstreaminterface> )

find_package (Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)

# Copy the api folder to the bin output directory so they can be easily used
add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/api/ ${CMAKE_SOURCE_DIR}/bin/api/decoders/${PROJECT_NAME}/api/)

set_property(TARGET ${PROJECT_NAME}   PROPERTY FOLDER "decoders")
set_property(TARGET objdynamicdecoder PROPERTY FOLDER "decoders")

if (WIN32)
    add_compile_definitions(DECODERS_EXPORTS)
else()
    add_compile_definitions(DECODERS_STATIC_DEFINE)
endif()

target_include_directories(
    objdynamicdecoder PUBLIC src
    objdynamicdecoder PUBLIC api
    objdynamicdecoder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    objdynamicdecoder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../
)

target_link_libraries(objdynamicdecoder PRIVATE ${CONAN_LIBS})
