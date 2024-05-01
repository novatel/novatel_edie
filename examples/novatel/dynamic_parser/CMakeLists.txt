add_executable(dynamic_parser dynamic_parser.cpp)
target_link_libraries(dynamic_parser common decoders_dynamic_library novatel stream_interface)
set_property(TARGET dynamic_parser PROPERTY FOLDER "examples")
