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

add_compile_definitions(VERSION_INFO=${SKBUILD_PROJECT_VERSION})

nanobind_add_module(hw_interface_bindings STABLE_ABI NB_STATIC LTO
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/hw_interface.cpp
)
nanobind_add_module(decoders_bindings STABLE_ABI NB_STATIC LTO
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings/decoders.cpp
)

target_link_libraries(hw_interface_bindings PRIVATE EDIE::stream_interface EDIE::novatel)
target_link_libraries(decoders_bindings PRIVATE EDIE::stream_interface EDIE::novatel)

if(DEFINED SKBUILD_PROJECT_NAME)
    set(PYTHON_INSTALL_DIR ${SKBUILD_PROJECT_NAME})
elseif(NOT DEFINED PYTHON_INSTALL_DIR)
    set(PYTHON_INSTALL_DIR lib/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages/novatel_edie)
endif()

install(TARGETS hw_interface_bindings decoders_bindings
    LIBRARY DESTINATION ${PYTHON_INSTALL_DIR}
    COMPONENT python)
install(DIRECTORY novatel_edie/
        DESTINATION ${PYTHON_INSTALL_DIR}
        COMPONENT python)
install(FILES ${CMAKE_SOURCE_DIR}/database/messages_public.json
        DESTINATION ${PYTHON_INSTALL_DIR}
        COMPONENT python)
