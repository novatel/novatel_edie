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
if(DEFINED SKBUILD_PROJECT_NAME)
    set(PYTHON_INSTALL_DIR ${SKBUILD_PROJECT_NAME})
elseif(NOT DEFINED PYTHON_INSTALL_DIR)
    set(PYTHON_INSTALL_DIR lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages/novatel_edie)
endif()

# Low level equivalent of add module
set(NANOBIND_LIB_NAME "nanobind" CACHE STRING "Name of the nanobind shared library")
nanobind_build_library(${NANOBIND_LIB_NAME})
set_target_properties(${NANOBIND_LIB_NAME} PROPERTIES FOLDER python_bindings)
if(MSVC)
    # Nanobind already explicity marks symbols for MSVC builds and the auto-gen consistently crashes
    set_target_properties(${NANOBIND_LIB_NAME} PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS OFF
    )
endif()


add_library(python_common SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_common/bindings/init_bindings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_common/bindings/exceptions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_common/bindings/common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_common/bindings/logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_common/bindings/field_objects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_common/bindings/internal/logger_tester.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_common/bindings/message_database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_common/bindings/message_db_singleton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_common/bindings/unknown_bytes.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_common/bindings/framer_manager.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_common/implementations/message_db_singleton.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_common/implementations/field_objects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_common/implementations/message_database.cpp
)
target_include_directories(python_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(python_common PUBLIC ${NANOBIND_LIB_NAME})
target_link_libraries(python_common PUBLIC decoders_common)
nanobind_disable_stack_protector(python_common)
nanobind_compile_options(python_common)
nanobind_musl_static_libcpp(python_common)
nanobind_extension(python_common)

set_target_properties(python_common PROPERTIES
    FOLDER "python_bindings"
)


add_library(python_oem MODULE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_oem/implementations/message_database.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_oem/bindings/init_bindings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_oem/bindings/commander.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_oem/bindings/file_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_oem/bindings/filter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_oem/bindings/framer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_oem/bindings/decoder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_oem/bindings/oem_common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_oem/bindings/init_modules.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_oem/bindings/parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_oem/bindings/range_decompressor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_oem/bindings/rxconfig_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_oem/bindings/py_message_objects.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/py_oem/bindings/internal/decoder_tester.cpp
)
target_link_libraries(python_oem PRIVATE decoders_common)
target_link_libraries(python_oem PRIVATE ${NANOBIND_LIB_NAME})
target_link_libraries(python_oem PRIVATE python_common)
target_link_libraries(python_oem PRIVATE oem_decoder)
nanobind_opt_size(python_oem)
nanobind_lto(python_oem)
nanobind_set_visibility(python_oem)
nanobind_strip(python_oem)
nanobind_disable_stack_protector(python_oem)
nanobind_extension(python_oem)
nanobind_compile_options(python_oem)
nanobind_link_options(python_oem)
nanobind_musl_static_libcpp(python_oem)


set_target_properties(python_oem PROPERTIES FOLDER python_bindings)
target_include_directories(python_oem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)


target_compile_definitions(python_oem PRIVATE
    VERSION_INFO=${SKBUILD_PROJECT_VERSION}
    # https://github.com/boostorg/system/issues/32#issuecomment-462912013
    HAVE_SNPRINTF
)

# Copy dependent DLLs to python_common output directory for stub generation
if(WIN32)
    add_custom_command(TARGET python_common POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:decoders_common>
            $<TARGET_FILE:common>
            $<TARGET_FILE:oem_decoder>
            $<TARGET_FILE:${NANOBIND_LIB_NAME}>
            $<TARGET_FILE_DIR:python_common>
        COMMENT "Copying dependent DLLs to python_common output directory"
    )
endif()

add_custom_target(
    common_stubs ALL
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/create_binding_stubs.py
    --import $<TARGET_FILE_DIR:python_common>
    -m python_common
    -o ${CMAKE_CURRENT_BINARY_DIR}/stubs/python_common.pyi
    DEPENDS python_common decoders_common
)

add_custom_target(
    oem_stubs ALL
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/create_binding_stubs.py
    --import $<TARGET_FILE_DIR:python_common>
    --pattern-file ${CMAKE_CURRENT_SOURCE_DIR}/stubgen_pattern.txt
    -d python_common
    -m python_oem
    -o ${CMAKE_CURRENT_BINARY_DIR}/stubs/python_oem.pyi
    DEPENDS python_oem common_stubs
)

add_custom_target(
    dynamic_stubs ALL
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/novatel_edie_files/stubgen.py ${CMAKE_SOURCE_DIR}/database/database.json ${CMAKE_CURRENT_BINARY_DIR}/stubs
    COMMENT "Generating dynamic stubs"
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/novatel_edie_files/stubgen.py
        ${CMAKE_SOURCE_DIR}/database/database.json
)

set_target_properties(dynamic_stubs PROPERTIES FOLDER python_bindings)
set_target_properties(python_common decoders_common oem_decoder python_oem common PROPERTIES INSTALL_RPATH "$ORIGIN")

message(STATUS "PYTHON_INSTALL_DIR: ${PYTHON_INSTALL_DIR}")

install(TARGETS python_common
    RUNTIME DESTINATION ${PYTHON_INSTALL_DIR} COMPONENT python
    LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}
    COMPONENT python)
install(TARGETS common decoders_common  ${NANOBIND_LIB_NAME}
    RUNTIME DESTINATION ${PYTHON_INSTALL_DIR} COMPONENT python
    LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}
    COMPONENT python)
install(TARGETS oem_decoder python_oem
    RUNTIME DESTINATION ${PYTHON_INSTALL_DIR} COMPONENT python
    LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}
    COMPONENT python)


# Install novatel_edie files
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/novatel_edie_files/
        DESTINATION ${PYTHON_INSTALL_DIR}
        COMPONENT python)
# Install built-in database file
install(FILES ${CMAKE_SOURCE_DIR}/database/database.json
        DESTINATION ${PYTHON_INSTALL_DIR}
        COMPONENT python)
# Install the generated stubs
install (DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/stubs/
        DESTINATION ${PYTHON_INSTALL_DIR}
        COMPONENT python)
