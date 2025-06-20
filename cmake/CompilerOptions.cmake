if(MSVC)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
    add_definitions(-DWIN32 -D_WINDOWS)
    add_compile_options(
        /W4     # Enable high warning level
        /GR     # Enable RTTI (Runtime Type Information
        /EHsc   # Enable C++ exception handling
        /utf-8  # Force UTF-8 source encoding
        /wd4244 # Disabled Warning: Conversion from 'type1' to 'type2', possible loss of data
        /wd4293 # Disabled Warning: Shift count negative or too big, undefined behavior
        /wd4702 # Disabled Warning: Unreachable code detected
        /wd4996 # Disabled Warning: Deprecated function or unsafe CRT library function
    )
    add_compile_options("$<$<CONFIG:Release>:/Ox;/Ob2>")
    if(WARNINGS_AS_ERRORS)
        add_compile_options(/WX)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
    add_compile_options(-Wall -Wextra -pedantic -Wno-format-security)
    add_compile_options("$<$<CONFIG:Release>:-O3>")

    if(CMAKE_HOST_WIN32)
        add_compile_options($<$<CONFIG:Debug>:-Wa,-mbig-obj>)
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-Wno-format-truncation)
    endif()

    if(WARNINGS_AS_ERRORS)
        add_compile_options(-Werror)
    endif()

    if(COVERAGE)
        message("Coverage is On")
        add_compile_options(--coverage)
        add_link_options(--coverage)
    endif()
else()
    message(WARNING "Unable to identify compiler.")
endif()
