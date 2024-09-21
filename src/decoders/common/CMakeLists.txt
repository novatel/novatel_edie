set(TARGET_NAME "decoders_common")
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(${TARGET_NAME} STATIC ${SOURCES})
add_library(novatel_edie::${TARGET_NAME} ALIAS ${TARGET_NAME})
set_target_properties(${TARGET_NAME} PROPERTIES
    OUTPUT_NAME "edie_${TARGET_NAME}"
    FOLDER "decoders"
)
target_include_directories(${TARGET_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(${TARGET_NAME} PUBLIC cxx_std_17)

find_package(nlohmann_json REQUIRED CONFIG)
target_link_libraries(${TARGET_NAME} PUBLIC
    novatel_edie::common
    nlohmann_json::nlohmann_json
)
