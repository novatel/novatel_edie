set(TARGET_NAME "stream_interface_test")
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(${TARGET_NAME} ${SOURCES})
set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "hw_interface/tests")
find_package(GTest REQUIRED CONFIG)
target_link_libraries(${TARGET_NAME} PUBLIC
    novatel_edie::stream_interface
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME} ${CMAKE_SOURCE_DIR})
