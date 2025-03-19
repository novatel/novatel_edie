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

nanobind_add_module(python_bindings
    STABLE_ABI NB_STATIC LTO
    NB_DOMAIN novatel_edie
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/bindings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/commander.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/exceptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/file_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/framer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/message_db_singleton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/message_database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/decoder_tester.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/oem_common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/init_modules.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/range_decompressor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/rxconfig_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/py_message_objects.cpp
)

set_target_properties(python_bindings PROPERTIES OUTPUT_NAME bindings)
target_include_directories(python_bindings PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/bindings)
target_link_libraries(python_bindings PRIVATE oem_decoder)

target_compile_definitions(python_bindings PRIVATE
    VERSION_INFO=${SKBUILD_PROJECT_VERSION}
    # https://github.com/boostorg/system/issues/32#issuecomment-462912013
    HAVE_SNPRINTF
)

nanobind_add_stub(
    bindings_stub
    MODULE bindings
    OUTPUT stubs/bindings.pyi
    PYTHON_PATH $<TARGET_FILE_DIR:python_bindings>
    DEPENDS bindings
)

add_custom_target(
    dynamic_stubs ALL
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/novatel_edie_files/stubgen.py ${CMAKE_SOURCE_DIR}/database/database.json ${CMAKE_CURRENT_BINARY_DIR}/stubs
    COMMENT "Generating dynamic stubs"
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/novatel_edie_files/stubgen.py
        ${CMAKE_SOURCE_DIR}/database/database.json
)

if(DEFINED SKBUILD_PROJECT_NAME)
    set(PYTHON_INSTALL_DIR ${SKBUILD_PROJECT_NAME})
elseif(NOT DEFINED PYTHON_INSTALL_DIR)
    set(PYTHON_INSTALL_DIR lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages/novatel_edie)
endif()

install(TARGETS python_bindings
    LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}
    COMPONENT python)
install(DIRECTORY novatel_edie_files/
        DESTINATION ${PYTHON_INSTALL_DIR}
        COMPONENT python)
install(FILES ${CMAKE_SOURCE_DIR}/database/database.json
        DESTINATION ${PYTHON_INSTALL_DIR}
        COMPONENT python)
install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/stubs/
        DESTINATION ${PYTHON_INSTALL_DIR}
        COMPONENT python)
