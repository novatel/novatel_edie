set(TARGET_NAME "rxconfig_handler")
add_executable(${TARGET_NAME} ${TARGET_NAME}.cpp)
target_link_libraries(${TARGET_NAME} novatel_edie::oem_decoder novatel_edie::stream_interface)
set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "examples")
