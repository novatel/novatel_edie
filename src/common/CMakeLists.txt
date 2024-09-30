set(TARGET_NAME "common")
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(${TARGET_NAME} ${SOURCES})
set_target_properties(${TARGET_NAME} PROPERTIES
    OUTPUT_NAME "edie_${TARGET_NAME}"
    FOLDER "common"
)
target_include_directories(${TARGET_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(${TARGET_NAME} PUBLIC cxx_std_17)

find_package(spdlog REQUIRED CONFIG)
find_package(spdlog_setup REQUIRED MODULE)

target_link_libraries(${TARGET_NAME} PUBLIC
    spdlog_setup::spdlog_setup
    $<TARGET_NAME_IF_EXISTS:spdlog::spdlog>
    $<TARGET_NAME_IF_EXISTS:spdlog::spdlog_header_only>
)

install(TARGETS ${TARGET_NAME}
    EXPORT novatel_edie-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
