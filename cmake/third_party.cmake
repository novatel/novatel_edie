if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(USE_CONAN "Use Conan to automatically manage dependencies" TRUE)

set(CONAN_INSTALL_ARGS
    # Deploy the installed dependencies in the build dir for easier installation
    --build=missing --deployer=full_deploy --deployer-folder=${CMAKE_BINARY_DIR}
)
if(BUILD_SHARED_LIBS)
    # Statically linking against spdlog causes its singleton logger registry to be
    # re-instantiated in each shared library and executable that links against it.
    list(APPEND CONAN_INSTALL_ARGS
        -o spdlog/*:shared=True -o fmt/*:shared=True
    )
endif()

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
