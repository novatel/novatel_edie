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
    add_custom_target(copy_third_party_dlls ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${third_party_base}/$<CONFIG>" "${target_dir}"
        COMMENT "Copying third-party shared libraries to ${target_dir}")

endfunction()
