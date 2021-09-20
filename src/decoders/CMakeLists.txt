cmake_minimum_required(VERSION 3.12.4)
project(Decoders VERSION 1.0.0)

add_subdirectory(common)
add_subdirectory(jsoninterface)
add_subdirectory(novatel)
add_subdirectory(dynamic_library)


if(VERSION)
	execute_process(COMMAND ${CMAKE_COMMAND}
	    -D VERSION=${VERSION} 
	    -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/version.h.in 
	    -D DST=${CMAKE_CURRENT_SOURCE_DIR}/version.h 
	    -D GIT_EXECUTABLE=${GIT_EXECUTABLE} 
	    -P ${CMAKE_SOURCE_DIR}/cmake/GenerateVersionHeader.cmake)
else()
	# Find Git package, if not need to install manually or through .yml file
	find_package(Git)
	if(Git_FOUND)
	  message("Git found: ${GIT_EXECUTABLE}")
	endif()

	if(GIT_EXECUTABLE)
	    execute_process(COMMAND ${CMAKE_COMMAND}
	        -D SRC=${CMAKE_CURRENT_SOURCE_DIR}/version.h.in 
	        -D DST=${CMAKE_CURRENT_SOURCE_DIR}/version.h 
	        -D GIT_EXECUTABLE=${GIT_EXECUTABLE} 
	        -P ${CMAKE_SOURCE_DIR}/cmake/GenerateVersionHeader.cmake)
	endif()
endif()


