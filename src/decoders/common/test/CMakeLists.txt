set(TARGET_NAME "decoders_common_test")
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(${TARGET_NAME} ${SOURCES})
set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "decoders/tests")
target_link_libraries(${TARGET_NAME} PUBLIC decoders_common GTest::gtest GTest::gtest_main)
gtest_discover_tests(${TARGET_NAME}
    TEST_PREFIX ${TARGET_NAME}.
)
install(TARGETS ${TARGET_NAME} DESTINATION tests/novatel)
