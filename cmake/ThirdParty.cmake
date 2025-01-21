include(CMakeDependentOption)

if(CMAKE_TOOLCHAIN_FILE MATCHES "conan_toolchain.cmake")
    set(CONAN_ALREADY_ACTIVE 1)
endif()

cmake_dependent_option(USE_CONAN "Use Conan to automatically manage dependencies" ON
    "NOT DEFINED VCPKG_TOOLCHAIN AND NOT CONAN_ALREADY_ACTIVE" OFF)

if(USE_CONAN)
    if(CMAKE_VERSION VERSION_LESS 3.24)
        message(FATAL_ERROR "Automatic Conan integration requires CMake 3.24 or later.")
    endif()
    include("${CMAKE_CURRENT_LIST_DIR}/SetDefaultProfile.cmake")
    # Set build cppstd for patchelf
    set(CONAN_INSTALL_ARGS --build missing --settings:build compiler.cppstd=17 CACHE INTERNAL "")
    list(APPEND CMAKE_PROJECT_TOP_LEVEL_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/conan_provider.cmake)
endif()

# Copy third-party shared libs to the build directory for tests
function(copy_third_party_shared_libs target_dir)
    if(USE_CONAN)
        get_property(conan_generators_folder GLOBAL PROPERTY CONAN_GENERATORS_FOLDER)
        include("${conan_generators_folder}/conan_runtime_paths.cmake")
    endif()
    if(NOT DEFINED CONAN_RUNTIME_LIB_DIRS)
        if(USE_CONAN)
            message(FATAL_ERROR "Failed to load CONAN_RUNTIME_LIB_DIRS")
        endif()
        # TODO: Add support for vcpkg
        message(STATUS "Not using Conan, skipping copying third-party shared libraries.")
        return()
    endif()

    message(STATUS "Copying third-party shared libraries to ${target_dir}...")

    if(WIN32)
        set(pattern "*.dll")
    elseif(APPLE)
        set(pattern "*.dylib")
    else()
        set(pattern "*.so*")
    endif()

    set(copied_files)
    foreach(path ${CONAN_RUNTIME_LIB_DIRS})
        message(STATUS "Copying shared libraries from ${path}")
        file(GLOB libs "${path}/${pattern}")
        file(COPY ${libs} DESTINATION "${target_dir}")
        list(APPEND copied_files ${libs})
    endforeach()

    # Set RPATH to $ORIGIN for the copied libraries
    if(NOT WIN32)
        foreach(lib ${copied_files})
            get_filename_component(lib_name ${lib} NAME)
            if(APPLE)
                execute_process(COMMAND install_name_tool -add_rpath @loader_path "${target_dir}/${lib_name}"
                    COMMAND_ERROR_IS_FATAL ANY)
            else()
                find_program(PATCHELF_EXECUTABLE patchelf REQUIRED)
                execute_process(COMMAND "${PATCHELF_EXECUTABLE}" --set-rpath \$ORIGIN "${target_dir}/${lib_name}"
                    COMMAND_ERROR_IS_FATAL ANY)
            endif()
        endforeach()
    endif()
endfunction()
