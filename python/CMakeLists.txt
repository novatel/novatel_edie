find_package(Python 3.8
    REQUIRED COMPONENTS Interpreter Development.Module
    OPTIONAL_COMPONENTS Development.SABIModule)

if(NOT DEFINED nanobind_DIR)
    execute_process(COMMAND ${Python_EXECUTABLE} -c "import nanobind; print(nanobind.cmake_dir())"
        RESULT_VARIABLE cmd_result
        OUTPUT_VARIABLE nanobind_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (NOT cmd_result EQUAL 0)
        unset(nanobind_DIR)
    endif()
endif()
find_package(nanobind CONFIG REQUIRED)

if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    # Disable CMAKE_CXX_EXTENSIONS=OFF warnings for nanobind
    add_compile_options(-Wno-pedantic -Wno-zero-length-array -Wno-nested-anon-types)
endif()

add_compile_definitions(VERSION_INFO=${SKBUILD_PROJECT_VERSION})

nanobind_add_module(stream_interface_bindings STABLE_ABI NB_STATIC LTO
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/stream_interface.cpp
)
nanobind_add_module(decoders_bindings STABLE_ABI NB_STATIC LTO
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/decoders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/common/common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/common/json_reader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/novatel/commander.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/novatel/encoder.cpp
)

target_include_directories(stream_interface_bindings PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bindings)
target_include_directories(decoders_bindings PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bindings)

target_link_libraries(stream_interface_bindings PRIVATE stream_interface)
target_link_libraries(decoders_bindings PRIVATE oem_decoder)

if(DEFINED SKBUILD_PROJECT_NAME)
    set(PYTHON_INSTALL_DIR ${SKBUILD_PROJECT_NAME})
elseif(NOT DEFINED PYTHON_INSTALL_DIR)
    set(PYTHON_INSTALL_DIR lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages/novatel_edie)
endif()

install(TARGETS stream_interface_bindings decoders_bindings
    LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}
    COMPONENT python)
install(DIRECTORY novatel_edie/
        DESTINATION ${PYTHON_INSTALL_DIR}
        COMPONENT python)
install(FILES ${CMAKE_SOURCE_DIR}/database/messages_public.json
        DESTINATION ${PYTHON_INSTALL_DIR}
        COMPONENT python)
