set(TARGET_NAME "novatel_test")
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(${TARGET_NAME} ${SOURCES})
set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "decoders/tests")
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../../)
find_package(GTest REQUIRED CONFIG)
target_link_libraries(${TARGET_NAME} PUBLIC
    novatel common stream_interface
    GTest::gtest GTest::gtest_main
)
add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME} ${CMAKE_SOURCE_DIR})
