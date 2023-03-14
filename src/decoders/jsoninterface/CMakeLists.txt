cmake_minimum_required(VERSION 3.10.2)
project(JsonInterface VERSION 1.0.0)

file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.c* ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h* ${CMAKE_CURRENT_SOURCE_DIR}/api/*.h*)
set(JASON_SOURCES)
LIST(APPEND JASON_SOURCES ${SOURCES})

if(WINDOWS)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NOVATEL_SOURCES})
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}-${ARCH})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}-${ARCH})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}-${ARCH})

#Creating an object file which can be used for both static and shared build without recompilation.
add_library(objjsoninterface OBJECT ${JASON_SOURCES})

#Creating a static lib, with the project name, using available object file, without recompilation. 
add_library(${PROJECT_NAME} STATIC $<TARGET_OBJECTS:objjsoninterface>)

#Copy the api folder to the bin output directory so they can be easily used
add_custom_command(TARGET ${PROJECT_NAME} 
                   POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory  ${CMAKE_CURRENT_SOURCE_DIR}/api/ ${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}/api/)

#Creating a shared/dynamic build and adding it`s project to "decoders" folder structure in VS.
if(CMAKE_LIB_SHARED)
add_library(DynamicJsonInterface SHARED $<TARGET_OBJECTS:objjsoninterface>)
set_property(TARGET DynamicJsonInterface PROPERTY FOLDER "decoders")
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER "decoders")
set_property(TARGET objjsoninterface PROPERTY FOLDER "decoders")

target_include_directories(objjsoninterface PUBLIC src)
target_include_directories(objjsoninterface PUBLIC api)
target_include_directories(objjsoninterface PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../)

if(LINUX)
# List the headers we want to declare as public for installation.
file(GLOB_RECURSE MY_PUBLIC_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/api/*.h*)
set_target_properties(${PROJECT_NAME} PROPERTIES PUBLIC_HEADER "${MY_PUBLIC_HEADERS}")

#Install the shared/dynamic lib.
if(CMAKE_LIB_SHARED)
install(TARGETS ${PROJECT_NAME} DynamicJsonInterface
        ARCHIVE DESTINATION usr/lib
        LIBRARY DESTINATION usr/lib
        PUBLIC_HEADER DESTINATION usr/include/novatel/edie/decoders/jsoninterface)
else()		
install(TARGETS ${PROJECT_NAME}
        ARCHIVE DESTINATION usr/lib
        LIBRARY DESTINATION usr/lib
        PUBLIC_HEADER DESTINATION usr/include/novatel/edie/decoders/jsoninterface)
endif()
		
endif()
