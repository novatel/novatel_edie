include(CMakeDependentOption)

if(CMAKE_TOOLCHAIN_FILE MATCHES "conan_toolchain.cmake")
    set(CONAN_ALREADY_ACTIVE 1)
endif()

cmake_dependent_option(USE_CONAN "Use Conan to automatically manage dependencies" ON
    "NOT DEFINED VCPKG_TOOLCHAIN AND NOT CONAN_ALREADY_ACTIVE" OFF)

# Set the default profile for Conan if not already set

set(CONAN_BUILD_SHARED "False")
if(BUILD_SHARED_LIBS)
   set(CONAN_BUILD_SHARED "True")
endif()


if(USE_CONAN)
    if(CMAKE_VERSION VERSION_LESS 3.24)
        message(FATAL_ERROR "Automatic Conan integration requires CMake 3.24 or later.")
    endif()
    include("${CMAKE_CURRENT_LIST_DIR}/SetDefaultProfile.cmake")
    # Set build cppstd for patchelf
    set(CONAN_INSTALL_ARGS --build missing -o shared=${CONAN_BUILD_SHARED} --settings:build compiler.cppstd=17  --conf=user.novatel_edie:cmake_driven=True CACHE INTERNAL "")
    list(APPEND CMAKE_PROJECT_TOP_LEVEL_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/conan_provider.cmake)
endif()

# Capture this at parse time, not inside the function
set(_THIRD_PARTY_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE INTERNAL "")

# Copy third-party shared libs to the build directory for tests
function(copy_third_party_shared_libs output_dir_base)
    # Determine where to find conan provided shared libraries
    if(CONAN_ALREADY_ACTIVE)
        set(source_dir "${CONAN_BUILD_DIR}/third_party_libs")
    elseif(USE_CONAN)
        set(source_dir "${CMAKE_BINARY_DIR}/conan/build/third_party_libs")
    else()
        message(STATUS "Not using Conan, skipping copying third-party shared libraries.")
        return()
    endif()

    # Collect build configurations from existing directories in source_dir
    file(GLOB config_dirs LIST_DIRECTORIES true "${source_dir}/*")
    set(COPY_BUILD_CONFIGURATIONS "")
    foreach(config_dir IN LISTS config_dirs)
        if(IS_DIRECTORY "${config_dir}")
            get_filename_component(config_name "${config_dir}" NAME)
            list(APPEND COPY_BUILD_CONFIGURATIONS "${config_name}")
        endif()
    endforeach()

    # Copy and adjust rpaths for libraries from each configuration
    foreach(config IN LISTS COPY_BUILD_CONFIGURATIONS)
        set(config_source_dir "${source_dir}/${config}")
        set(config_target_dir "${output_dir_base}-${config}")
        file(MAKE_DIRECTORY "${config_target_dir}")
        file(GLOB lib_files "${config_source_dir}/*")
        foreach(lib_file IN LISTS lib_files)
            file(COPY "${lib_file}" DESTINATION "${config_target_dir}")
        endforeach()
        message(STATUS "Copied third-party libraries from ${config_source_dir} to ${config_target_dir}")

        # Set rpath for each copied library (non-Windows only)
        if(NOT WIN32)
            foreach(lib_file IN LISTS lib_files)
                get_filename_component(lib_name "${lib_file}" NAME)
                set(target_lib "${config_target_dir}/${lib_name}")
                if(APPLE)
                    execute_process(COMMAND install_name_tool -add_rpath @loader_path "${target_lib}"
                        RESULT_VARIABLE result)
                    if(NOT result EQUAL 0)
                        message(WARNING "Failed to set rpath for ${target_lib}")
                    endif()
                else()
                    find_program(PATCHELF_EXECUTABLE patchelf)
                    if(PATCHELF_EXECUTABLE)
                        execute_process(COMMAND "${PATCHELF_EXECUTABLE}" --set-rpath $ORIGIN "${target_lib}"
                            RESULT_VARIABLE result)
                        if(NOT result EQUAL 0)
                            message(WARNING "Failed to set rpath for ${target_lib}")
                        endif()
                    else()
                        message(WARNING "patchelf not found, cannot set rpath for ${target_lib}")
                    endif()
                endif()
            endforeach()
        endif()
    endforeach()
endfunction()
