get_filename_component(SRC_DIR ${SRC} DIRECTORY)

# Generate a git-describe version string from Git repository tags

if(GIT_EXECUTABLE AND NOT DEFINED VERSION)

  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --always --tags --dirty --match "v*"
    WORKING_DIRECTORY ${SRC_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
    RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
    OUTPUT_VARIABLE GIT_FULL_SHA
    RESULT_VARIABLE GIT_SHA_ERROR_CODE
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  
  if(NOT GIT_BRANCH)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} branch --show-current
      OUTPUT_VARIABLE GIT_CURRENT_BRANCH
      RESULT_VARIABLE GIT_BRANCH_ERROR_CODE
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  else()
    set(GIT_CURRENT_BRANCH ${GIT_BRANCH})
    set(GIT_BRANCH_ERROR_CODE 0)
  endif()
  
  if (WIN32)
      execute_process(
        COMMAND cmd /c echo %date:~10,4%-%date:~7,2%-%date:~4,2%T%time:~0,2%:%time:~3,2%:%time:~6,2%
        OUTPUT_VARIABLE BUILDTIMESTAMP
        RESULT_VARIABLE GIT_TIMESTAMP_ERROR_CODE
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
  else()
    execute_process(
        COMMAND date +%Y-%m-%dT%H:%M:%S
        OUTPUT_VARIABLE BUILDTIMESTAMP
        RESULT_VARIABLE GIT_TIMESTAMP_ERROR_CODE
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
  endif()
  
  execute_process(
    COMMAND ${GIT_EXECUTABLE} diff --shortstat
    OUTPUT_VARIABLE GIT_DIRTY_STATUS
    RESULT_VARIABLE GIT_DIFF_ERROR_CODE
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if(NOT GIT_DESCRIBE_ERROR_CODE)
    set(VERSION ${GIT_DESCRIBE_VERSION})
  endif()

  if(NOT GIT_SHA_ERROR_CODE)
    set(GIT_SHA ${GIT_FULL_SHA})
  endif()

  if(NOT GIT_BRANCH_ERROR_CODE)
    set(GIT_BRANCH ${GIT_CURRENT_BRANCH})  
  endif()

  if(NOT GIT_TIMESTAMP_ERROR_CODE)
    set(BUILD_TIMESTAMP ${BUILDTIMESTAMP})  
  endif()  

  if((NOT GIT_DIRTY_STATUS) AND (NOT GIT_DIFF_ERROR_CODE))
    set(GIT_IS_DIRTY "false")
  else()  
    set(GIT_IS_DIRTY "true")    
  endif() 
  
endif()

# Final fallback: Just use a bogus version string that is semantically older
# than anything else and spit out a warning to the developer.

if(NOT DEFINED VERSION)
  set(VERSION v0.0.0-unknown)
  message(WARNING "Failed to determine VERSION from repository tags. Using default version \"${VERSION}\".")
endif()

if(NOT DEFINED GIT_SHA)
  set(GIT_SHA 0000000000000000)
  message(WARNING "Failed to determine GIT_SHA from repository tags. Using default version \"${GIT_SHA}\".")
endif()

if(NOT DEFINED GIT_BRANCH)
  set(GIT_BRANCH Error)
  message(WARNING "Failed to determine GIT_BRANCH from repository tags. Using default version \"${GIT_BRANCH}\".")
endif()

if(NOT DEFINED GIT_IS_DIRTY)
  set(GIT_IS_DIRTY FALSE)
  message(WARNING "Failed to determine GIT_IS_DIRTY from repository tags. Using default version \"${GIT_IS_DIRTY}\".")
endif()

if(NOT DEFINED BUILD_TIMESTAMP)
  set(BUILD_TIMESTAMP 0000-00-00T00:00:00)
  message(WARNING "Failed to determine BUILD_TIMESTAMP from repository tags. Using default version \"${BUILD_TIMESTAMP}\".")
endif()

message(DEBUG "Version \"${VERSION}\".")
message(DEBUG "SHA     \"${GIT_SHA}\".")
message(DEBUG "Branch  \"${GIT_BRANCH}\".")
message(DEBUG "Status  \"${GIT_IS_DIRTY}\".")
message(DEBUG "Time    \"${BUILD_TIMESTAMP}\".")

configure_file(${SRC} ${DST} @ONLY)
