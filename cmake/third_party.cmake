if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(USE_CONAN "Use Conan to automatically manage dependencies" TRUE)

if(USE_CONAN AND NOT DEFINED VCPKG_TOOLCHAIN AND NOT CMAKE_TOOLCHAIN_FILE MATCHES "conan_toolchain.cmake")
    if(CMAKE_VERSION GREATER_EQUAL 3.24)
        list(APPEND CMAKE_PROJECT_TOP_LEVEL_INCLUDES ${CMAKE_CURRENT_LIST_DIR}/conan_provider.cmake)
    else()
        message(WARNING
            "CMake 3.24 or greater is required to install Conan dependencies automatically. "
            "You will have to run 'conan install . ${CONAN_INSTALL_ARGS}' manually in the source directory instead."
        )
        # To use the output from the Conan CMakeDeps generator
        list(PREPEND CMAKE_PREFIX_PATH
            ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/generators
            ${CMAKE_CURRENT_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}/generators
        )
    endif()
endif()
