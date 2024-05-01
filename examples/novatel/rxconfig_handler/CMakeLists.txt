add_executable(rxconfig_handler rxconfig_handler.cpp)
target_link_libraries(rxconfig_handler common novatel stream_interface)
set_property(TARGET rxconfig_handler PROPERTY FOLDER "examples")
