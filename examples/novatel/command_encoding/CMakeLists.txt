add_executable(command_encoding command_encoding.cpp)
target_link_libraries(command_encoding common novatel stream_interface)
set_property(TARGET command_encoding PROPERTY FOLDER "examples")
