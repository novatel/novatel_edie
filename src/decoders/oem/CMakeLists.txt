set(TARGET_NAME "oem_decoder")
file(GLOB_RECURSE SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(${TARGET_NAME} ${SOURCES})
set_target_properties(${TARGET_NAME} PROPERTIES
    OUTPUT_NAME "edie_${TARGET_NAME}"
    FOLDER "decoders"
)

find_package(simdjson REQUIRED)

target_link_libraries(${TARGET_NAME} PUBLIC common decoders_common simdjson::simdjson)
target_include_directories(${TARGET_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

install(TARGETS ${TARGET_NAME}
    EXPORT novatel_edie-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
