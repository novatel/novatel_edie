# Copy the C++ runtime DLL for non-MSVC compilers on Windows
function(copy_cpp_runtime_dlls target_dir)
    if(WIN32 AND NOT MSVC)
        if(NOT EXISTS CMAKE_CXX_COMPILER)
            find_program(CXX_COMPILER_PATH NAMES "${CMAKE_CXX_COMPILER}")
        else()
            set(CXX_COMPILER_PATH "${CMAKE_CXX_COMPILER}")
        endif()
        get_filename_component(COMPILER_BIN_DIR "${CXX_COMPILER_PATH}" DIRECTORY)
        foreach(stdcpp_library libstdc++-6.dll libc++.dll)
            if(EXISTS "${COMPILER_BIN_DIR}/${stdcpp_library}")
                file(COPY "${COMPILER_BIN_DIR}/${stdcpp_library}" DESTINATION "${target_dir}")
            endif()
        endforeach()
    endif()
endfunction()

# Set test environment for Windows to find DLLs at runtime
# Must be called after add_test() and uses OUTPUT_DIR_BASE from parent scope
function(set_test_dll_path test_name)
    if(WIN32)
        get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
        if(IS_MULTI_CONFIG)
            set_tests_properties(${test_name} PROPERTIES
                ENVIRONMENT "PATH=${OUTPUT_DIR_BASE}-$<CONFIG>;$ENV{PATH}"
            )
        else()
            set_tests_properties(${test_name} PROPERTIES
                ENVIRONMENT "PATH=${OUTPUT_DIR};$ENV{PATH}"
            )
        endif()
    endif()
endfunction()

# Generate and install nova_edie-config.cmake
function(install_novatel_edie_cmake_config)
    include(GNUInstallDirs)
    set(CMAKE_CONFIG_INSTALL_DIR lib/cmake/novatel_edie)

    install(TARGETS novatel_edie EXPORT novatel_edie-targets)
    install(EXPORT novatel_edie-targets
        NAMESPACE novatel_edie::
        FILE novatel_edie-targets.cmake
        DESTINATION ${CMAKE_CONFIG_INSTALL_DIR}
        COMPONENT novatel_edie)

    include(CMakePackageConfigHelpers)
    configure_package_config_file(cmake/novatel_edie-config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/novatel_edie-config.cmake"
        INSTALL_DESTINATION ${CMAKE_CONFIG_INSTALL_DIR}
        PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_DATADIR)

    write_basic_package_version_file(novatel_edie-config-version.cmake
        VERSION ${novatel_edie_VERSION}
        COMPATIBILITY SameMajorVersion)

    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/novatel_edie-config.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/novatel_edie-config-version.cmake"
        DESTINATION ${CMAKE_CONFIG_INSTALL_DIR}
        COMPONENT novatel_edie)
endfunction()
