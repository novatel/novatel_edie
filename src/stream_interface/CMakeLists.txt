set(TARGET_NAME "novatel_edie_stream_interface")
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(${TARGET_NAME} ${SOURCES})
add_library(${PROJECT_NAME}::stream_interface ALIAS ${TARGET_NAME})
target_include_directories(${TARGET_NAME} PUBLIC include)
target_link_libraries(${TARGET_NAME} PUBLIC novatel_edie::common)
set_target_properties(${TARGET_NAME} PROPERTIES
    FOLDER "stream_interface"
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
)
