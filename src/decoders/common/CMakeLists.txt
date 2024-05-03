set(TARGET_NAME "common")
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(${TARGET_NAME} STATIC ${SOURCES})
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/api/ ${CMAKE_SOURCE_DIR}/bin/api/decoders/${TARGET_NAME}/api/)
set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "decoders")
target_include_directories(${TARGET_NAME} PUBLIC api)

find_package(nlohmann_json REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
find_package(spdlog_setup REQUIRED CONFIG)

target_link_libraries(${TARGET_NAME} PUBLIC
    nlohmann_json::nlohmann_json
    spdlog_setup::spdlog_setup
    $<TARGET_NAME_IF_EXISTS:spdlog::spdlog>
    $<TARGET_NAME_IF_EXISTS:spdlog::spdlog_header_only>
)
