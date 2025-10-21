set(TARGET_NAME "oem_test")
file(GLOB SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
add_executable(${TARGET_NAME} ${SOURCES})
set_property(TARGET ${TARGET_NAME} PROPERTY FOLDER "decoders/tests")
find_package(GTest REQUIRED CONFIG)
target_link_libraries(${TARGET_NAME} PUBLIC
    oem_decoder
    GTest::gtest GTest::gtest_main
)
add_test(NAME ${TARGET_NAME} COMMAND ${TARGET_NAME})

# TODO: Re-enable this once deprecated wstring_convert function is removed
if(NOT MSVC)
    target_compile_options(oem_test PRIVATE -Wno-deprecated-declarations)
endif()

install(TARGETS ${TARGET_NAME} DESTINATION tests/novatel)
