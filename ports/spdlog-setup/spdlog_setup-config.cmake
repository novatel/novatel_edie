if(TARGET spdlog_setup::spdlog_setup)
  return()
endif()

add_library(spdlog_setup::spdlog_setup INTERFACE IMPORTED)
set(spdlog_setup_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../include")
set(spdlog_setup_INCLUDE_DIRS "${spdlog_setup_INCLUDE_DIR}")
set_target_properties(spdlog_setup::spdlog_setup PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${spdlog_setup_INCLUDE_DIRS}"
)
