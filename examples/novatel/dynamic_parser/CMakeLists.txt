set(TARGET_NAME "dynamic_parser")
add_executable(${TARGET_NAME} ${TARGET_NAME}.cpp)
target_link_libraries(${TARGET_NAME} common decoders_dynamic_library novatel stream_interface)
set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "examples")
