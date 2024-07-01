set(TARGET_NAME "novatel_edie_decoders")
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(${TARGET_NAME} ${SOURCES})
add_library(${PROJECT_NAME}::decoders ALIAS ${TARGET_NAME})
target_include_directories(${TARGET_NAME} PUBLIC include)
target_link_libraries(${TARGET_NAME} PUBLIC novatel_edie::common novatel_edie::stream_interface )
set_target_properties(${TARGET_NAME} PROPERTIES
    FOLDER "decoders"
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
)
