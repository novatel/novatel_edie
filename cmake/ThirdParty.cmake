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

# Copy third-party shared libs to the build directory for tests
# For multi-config generators, creates per-config custom targets
# For single-config generators, copies at configure time
function(copy_third_party_shared_libs target_dir_base)
    if(USE_CONAN)
        get_property(conan_generators_folder GLOBAL PROPERTY CONAN_GENERATORS_FOLDER)
        include("${conan_generators_folder}/conan_runtime_paths.cmake")
    endif()
    if(NOT DEFINED CONAN_RUNTIME_LIB_DIRS)
        if(USE_CONAN)
            message(FATAL_ERROR "Failed to load CONAN_RUNTIME_LIB_DIRS")
        endif()
        message(STATUS "Not using Conan, skipping copying third-party shared libraries.")
        return()
    endif()

    if(WIN32)
        set(pattern "*.dll")
    elseif(APPLE)
        set(pattern "*.dylib")
    else()
        set(pattern "*.so*")
    endif()

    get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(IS_MULTI_CONFIG)
        # Parse $<$<CONFIG:X>:path> expressions and group by config
        foreach(CONFIG IN ITEMS CMAKE_CONFIGURATION_TYPES)
            set(${CONFIG}_PATHS)
        endforeach()

        foreach(path ${CONAN_RUNTIME_LIB_DIRS})
            if(path MATCHES "\\$<\\$<CONFIG:([^>]+)>:([^>]+)>")
                list(APPEND ${CMAKE_MATCH_1}_PATHS "${CMAKE_MATCH_2}")
            elseif(NOT path MATCHES "\\$<")
                list(APPEND Debug_PATHS "${path}")
                list(APPEND Release_PATHS "${path}")
            endif()
        endforeach()

        foreach(CONFIG IN ITEMS CMAKE_CONFIGURATION_TYPES)
            set(target_dir "${target_dir_base}-${CONFIG}")
            set(copy_commands)
            foreach(path ${${CONFIG}_PATHS})
                if(EXISTS "${path}")
                    list(APPEND copy_commands COMMAND ${CMAKE_COMMAND} -E copy_directory "${path}" "${target_dir}")
                endif()
            endforeach()
            if(copy_commands)
                add_custom_target(copy_third_party_dlls_${CONFIG} ALL ${copy_commands}
                    COMMENT "Copying third-party shared libraries to ${target_dir}")
            endif()
        endforeach()
    else()
        # Single-config: copy at configure time
        message(STATUS "Copying third-party shared libraries to ${target_dir_base}...")
        set(copied_files)
        foreach(path ${CONAN_RUNTIME_LIB_DIRS})
            message(STATUS "Copying shared libraries from ${path}")
            file(GLOB libs "${path}/${pattern}")
            file(COPY ${libs} DESTINATION "${target_dir_base}")
            list(APPEND copied_files ${libs})
        endforeach()

        # Set RPATH for non-Windows
        if(NOT WIN32)
            foreach(lib ${copied_files})
                get_filename_component(lib_name ${lib} NAME)
                if(APPLE)
                    execute_process(COMMAND install_name_tool -add_rpath @loader_path "${target_dir_base}/${lib_name}"
                        COMMAND_ERROR_IS_FATAL ANY)
                else()
                    find_program(PATCHELF_EXECUTABLE patchelf REQUIRED)
                    execute_process(COMMAND "${PATCHELF_EXECUTABLE}" --set-rpath \$ORIGIN "${target_dir_base}/${lib_name}"
                        COMMAND_ERROR_IS_FATAL ANY)
                endif()
            endforeach()
        endif()
    endif()
endfunction()
