# CopyThirdPartyLibs.cmake
# This script is invoked at build time to copy third-party shared libraries
# and set their RPATH appropriately.
#
# Expected variables:
#   SOURCE_DIR - Directory containing the libraries to copy
#   TARGET_DIR - Destination directory for the copied libraries

# Copy all libs from a directory of libs
file(GLOB libs "${SOURCE_DIR}/*")
if(NOT libs)
    message(STATUS "No libraries found in ${SOURCE_DIR}")
    return()
endif()
message(STATUS "Copying shared libraries from ${SOURCE_DIR} to ${TARGET_DIR}")
file(COPY ${libs} DESTINATION "${TARGET_DIR}")

# Tell shared libs to look within the new directory for their dependencies
if(NOT WIN32)
    foreach(lib ${libs})
        get_filename_component(lib_name ${lib} NAME)
        set(target_lib "${TARGET_DIR}/${lib_name}")
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
