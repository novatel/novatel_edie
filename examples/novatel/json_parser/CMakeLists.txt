add_executable(json_parser json_parser.cpp)
target_link_libraries(json_parser common novatel stream_interface)
set_property(TARGET json_parser PROPERTY FOLDER "examples")
