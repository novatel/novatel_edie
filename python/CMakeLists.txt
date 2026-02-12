find_package(Python 3.8
    REQUIRED COMPONENTS Interpreter Development.Module
    OPTIONAL_COMPONENTS Development.SABIModule)

if(NOT DEFINED nanobind_DIR)
    message(${Python_EXECUTABLE})
    execute_process(COMMAND ${Python_EXECUTABLE} -c "import nanobind; print(nanobind.cmake_dir())"
        RESULT_VARIABLE cmd_result
        OUTPUT_VARIABLE nanobind_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (NOT cmd_result EQUAL 0)
        message(FATAL_ERROR "nanobind is not availible to the python executable: " ${Python_EXECUTABLE} "\n"
        "Change the python executable with the Python_EXECTUTABLE cache variable or with user.python:exe in your conan configuration.")
    endif()
endif()
find_package(nanobind CONFIG REQUIRED)

if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    # Disable CMAKE_CXX_EXTENSIONS=OFF warnings for nanobind
    add_compile_options(-Wno-pedantic -Wno-zero-length-array -Wno-nested-anon-types)
endif()

add_compile_definitions(VERSION_INFO=${SKBUILD_PROJECT_VERSION})
if(NOT DEFINED PYTHON_INSTALL_DIR)
    set(PYTHON_INSTALL_DIR lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages/novatel_edie)
endif()

# Use stable ABI builds if CPython >= 3.12 and Python::SABIModule is availible
if ((Python_VERSION VERSION_LESS 3.12) OR
    (NOT Python_INTERPRETER_ID STREQUAL "Python") OR
    (NOT TARGET Python::SABIModule))
    set(PYTHON_STABLE_ABI FALSE)
    message(STATUS "Not using Python stable ABI")
else()
    set(PYTHON_STABLE_ABI TRUE)
    message(STATUS "Using Python stable ABI")
endif()


if (PYTHON_STABLE_ABI)
    set(NANOBIND_LIB_NAME "nanobind-abi3" CACHE STRING "Name of the nanobind shared library")
else()
    set(NANOBIND_LIB_NAME "nanobind" CACHE STRING "Name of the nanobind shared library")
endif()
nanobind_build_library(${NANOBIND_LIB_NAME})
set_target_properties(${NANOBIND_LIB_NAME} PROPERTIES FOLDER python_bindings)
if (PYTHON_STABLE_ABI)
    # Tells python header files to link against stable ABI for Python 3.12+
    # 3(x03).12(x0C).0(x0000)
    target_compile_definitions(${NANOBIND_LIB_NAME} PUBLIC -DPy_LIMITED_API=0x030C0000)
endif()
if(MSVC)
    # Nanobind already explicity marks symbols for MSVC builds and the auto-gen consistently crashes
    set_target_properties(${NANOBIND_LIB_NAME} PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS OFF
    )
endif()


add_library(common_bindings SHARED
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
target_include_directories(common_bindings PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(common_bindings PUBLIC ${NANOBIND_LIB_NAME})
target_link_libraries(common_bindings PUBLIC decoders_common)
nanobind_disable_stack_protector(common_bindings)
nanobind_compile_options(common_bindings)
nanobind_musl_static_libcpp(common_bindings)
if (PYTHON_STABLE_ABI)
    nanobind_extension_abi3(common_bindings)
else()
    nanobind_extension(common_bindings)
endif()

set_target_properties(common_bindings PROPERTIES
    FOLDER "python_bindings"
)


add_library(oem_bindings MODULE
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
target_link_libraries(oem_bindings PRIVATE decoders_common)
target_link_libraries(oem_bindings PRIVATE ${NANOBIND_LIB_NAME})
target_link_libraries(oem_bindings PRIVATE common_bindings)
target_link_libraries(oem_bindings PRIVATE oem_decoder)
nanobind_opt_size(oem_bindings)
nanobind_lto(oem_bindings)
nanobind_set_visibility(oem_bindings)
nanobind_strip(oem_bindings)
nanobind_disable_stack_protector(oem_bindings)
if (PYTHON_STABLE_ABI)
    nanobind_extension_abi3(oem_bindings)
else()
    nanobind_extension(oem_bindings)
endif()

nanobind_compile_options(oem_bindings)
nanobind_link_options(oem_bindings)
nanobind_musl_static_libcpp(oem_bindings)


set_target_properties(oem_bindings PROPERTIES FOLDER python_bindings)
target_include_directories(oem_bindings PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)


target_compile_definitions(oem_bindings PRIVATE
    VERSION_INFO=${SKBUILD_PROJECT_VERSION}
    # https://github.com/boostorg/system/issues/32#issuecomment-462912013
    HAVE_SNPRINTF
)

# Copy dependent DLLs to common_bindings output directory for stub generation
if(WIN32)
    add_custom_command(TARGET common_bindings POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:decoders_common>
            $<TARGET_FILE:common>
            $<TARGET_FILE:oem_decoder>
            $<TARGET_FILE:${NANOBIND_LIB_NAME}>
            $<TARGET_FILE_DIR:common_bindings>
        COMMENT "Copying dependent DLLs to common_bindings output directory"
    )
endif()

add_custom_target(
    common_stubs ALL
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/create_binding_stubs.py
    --import $<TARGET_FILE_DIR:common_bindings>
    -m common_bindings
    -o ${CMAKE_CURRENT_BINARY_DIR}/stubs/common_bindings.pyi
    DEPENDS common_bindings decoders_common
)

add_custom_target(
    oem_stubs ALL
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/create_binding_stubs.py
    --import $<TARGET_FILE_DIR:common_bindings>
    --pattern-file ${CMAKE_CURRENT_SOURCE_DIR}/stubgen_pattern.txt
    -d common_bindings
    -m oem_bindings
    -o ${CMAKE_CURRENT_BINARY_DIR}/stubs/oem/oem_bindings.pyi
    DEPENDS oem_bindings common_stubs
)

add_custom_target(
    dynamic_stubs ALL
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/novatel_edie_files/stubgen.py ${CMAKE_SOURCE_DIR}/database/database.json ${CMAKE_CURRENT_BINARY_DIR}/stubs/oem
    COMMENT "Generating dynamic stubs"
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/novatel_edie_files/stubgen.py
        ${CMAKE_SOURCE_DIR}/database/database.json
)

set_target_properties(dynamic_stubs PROPERTIES FOLDER python_bindings)
set_target_properties(common_bindings decoders_common oem_decoder oem_bindings common PROPERTIES INSTALL_RPATH "$ORIGIN")

message(STATUS "PYTHON_INSTALL_DIR: ${PYTHON_INSTALL_DIR}")

install(TARGETS common_bindings
    RUNTIME DESTINATION ${PYTHON_INSTALL_DIR} COMPONENT python
    LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}
    COMPONENT python)
install(TARGETS common decoders_common  ${NANOBIND_LIB_NAME}
    RUNTIME DESTINATION ${PYTHON_INSTALL_DIR}/shared_libs COMPONENT python
    LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/shared_libs
    COMPONENT python)
install(TARGETS oem_decoder
    RUNTIME DESTINATION ${PYTHON_INSTALL_DIR}/shared_libs COMPONENT python
    LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/shared_libs
    COMPONENT python)
install(TARGETS oem_bindings
    RUNTIME DESTINATION ${PYTHON_INSTALL_DIR}/oem COMPONENT python
    LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}/oem
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
