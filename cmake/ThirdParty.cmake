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
function(copy_third_party_shared_libs target_dir)
    if(NOT USE_CONAN)
        message(STATUS "Not using Conan, skipping copying third-party shared libraries.")
        return()
    endif()

    set(third_party_base "${CMAKE_BINARY_DIR}/conan/build/third_party_libs")
    set(source_dir "${third_party_base}/$<CONFIG>")

    set(copied_files)
    message(STATUS "Copying shared libraries from ${source_dir}")
    file(GLOB libs "${source_dir}/*")
    file(COPY ${libs} DESTINATION "${target_dir}")
    foreach(lib ${libs})
        get_filename_component(lib_name ${lib} NAME)
        list(APPEND copied_files "${target_dir}/${lib_name}")
    endforeach()

    # Set RPATH to $ORIGIN for the copied libraries
    if(NOT WIN32)
        foreach(lib ${copied_files})
            if(APPLE)
                execute_process(COMMAND install_name_tool -add_rpath @loader_path "${lib}"
                    COMMAND_ERROR_IS_FATAL ANY)
            else()
                find_program(PATCHELF_EXECUTABLE patchelf REQUIRED)
                execute_process(COMMAND "${PATCHELF_EXECUTABLE}" --set-rpath \$ORIGIN "${lib}"
                    COMMAND_ERROR_IS_FATAL ANY)
            endif()
        endforeach()
    endif()
endfunction()
