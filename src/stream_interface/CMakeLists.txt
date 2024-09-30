set(TARGET_NAME "stream_interface")
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(${TARGET_NAME} STATIC ${SOURCES})
set_target_properties(${TARGET_NAME} PROPERTIES
    OUTPUT_NAME "edie_${TARGET_NAME}"
    FOLDER "hw_interface"
)
target_link_libraries(${TARGET_NAME} PRIVATE common)
target_include_directories(${TARGET_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
