set(TARGET_NAME "oem_decoder")
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(${TARGET_NAME} STATIC ${SOURCES})
add_library(novatel_edie::${TARGET_NAME} ALIAS ${TARGET_NAME})
set_target_properties(${TARGET_NAME} PROPERTIES
    OUTPUT_NAME "edie_${TARGET_NAME}"
    FOLDER "decoders"
)
target_link_libraries(${TARGET_NAME} PUBLIC novatel_edie::common decoders_common stream_interface)
target_include_directories(${TARGET_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
