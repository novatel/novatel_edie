set(TARGET_NAME "decoders_common_test")
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(${TARGET_NAME} ${SOURCES})
set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "decoders/tests")
find_package(GTest REQUIRED CONFIG)
target_link_libraries(${TARGET_NAME} PUBLIC decoders_common GTest::gtest GTest::gtest_main)
add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME} ${CMAKE_SOURCE_DIR})
# Copy runtime DLLs to the output directory on Windows
if(WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${TARGET_NAME}>
            $<TARGET_FILE_DIR:${TARGET_NAME}>
        COMMAND_EXPAND_LISTS
    )
endif()

install(TARGETS ${TARGET_NAME} DESTINATION tests/novatel)
