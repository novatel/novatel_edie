set(TARGET_NAME "novatel_edie_common")
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(${TARGET_NAME} ${SOURCES})
add_library(${PROJECT_NAME}::common ALIAS ${TARGET_NAME})
target_include_directories(${TARGET_NAME} PUBLIC include)
target_compile_features(${TARGET_NAME} PUBLIC cxx_std_17)
set_target_properties(${TARGET_NAME} PROPERTIES
    FOLDER "common"
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
)

find_package(nlohmann_json REQUIRED CONFIG)
find_package(spdlog REQUIRED CONFIG)
find_package(spdlog_setup REQUIRED CONFIG)

target_link_libraries(${TARGET_NAME} PUBLIC
    spdlog_setup::spdlog_setup
    nlohmann_json::nlohmann_json
)
if(TARGET spdlog::spdlog)
    target_link_libraries(${TARGET_NAME} PUBLIC spdlog::spdlog)
else()
    target_compile_definitions(${TARGET_NAME} PUBLIC spdlog::spdlog_header_only)
endif()
