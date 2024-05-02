set(TARGET_NAME "converter_components")
add_executable(${TARGET_NAME} ${TARGET_NAME}.cpp)
target_link_libraries(${TARGET_NAME} common novatel stream_interface)
set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "examples")
