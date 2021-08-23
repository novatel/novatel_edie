cmake_minimum_required(VERSION 3.12.4)
project(Novatel VERSION 1.0.0)


file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c* ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h* ${CMAKE_CURRENT_SOURCE_DIR}/api/*.h*)
set(NOVATEL_SOURCES)
LIST(APPEND NOVATEL_SOURCES ${SOURCES})

if(WINDOWS)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NOVATEL_SOURCES})
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}-${ARCH})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}-${ARCH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}-${ARCH})

#Creating an object file which can be used for both static and shared build without recompilation.
add_library(objnovatel OBJECT ${NOVATEL_SOURCES})

#Creating a static lib, with the project name, using available object file, without recompilation.
add_library(${PROJECT_NAME} STATIC $<TARGET_OBJECTS:objnovatel>)

#Copy the api folder to the bin output directory so they can be easily used
add_custom_command(TARGET ${PROJECT_NAME} 
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/api/ ${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}/api/)

#Creating a shared/dynamic build and adding it`s project to "decoders" folder structure in VS.
if(CMAKE_LIB_SHARED)
add_library(DynamicNovatel SHARED $<TARGET_OBJECTS:objnovatel> $<TARGET_OBJECTS:objcommon> $<TARGET_OBJECTS:objjsoninterface> $<TARGET_OBJECTS:objstreaminterface>)
set_property(TARGET DynamicNovatel PROPERTY FOLDER "decoders")
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "decoders")
set_property(TARGET objnovatel PROPERTY FOLDER "decoders")

target_include_directories(objnovatel PUBLIC src)
target_include_directories(objnovatel PUBLIC api)
target_include_directories(objnovatel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../)
target_include_directories(objnovatel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../common/api)

if(LINUX)
file(GLOB_RECURSE MY_PUBLIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/api/*.h*)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${MY_PUBLIC_HEADERS}")

#Install the shared/dynamic lib.
if(CMAKE_LIB_SHARED)
install(TARGETS ${PROJECT_NAME} DynamicNovatel
        ARCHIVE DESTINATION usr/lib
        LIBRARY DESTINATION usr/lib
        PUBLIC_HEADER DESTINATION usr/include/novatel/edie/decoders/novatel)
else()
install(TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION usr/lib
        LIBRARY DESTINATION usr/lib
        PUBLIC_HEADER DESTINATION usr/include/novatel/edie/decoders/novatel)
endif()
		
endif()
