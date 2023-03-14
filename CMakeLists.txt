cmake_minimum_required(VERSION 3.10.2)

project(EDIE VERSION 1.0.0)

set(CMAKE_VERBOSE_MAKEFILE on)
set (CMAKE_CXX_STANDARD 11)

include_directories(".")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Check platforms
if (CMAKE_HOST_WIN32)
   set(WINDOWS 1)
   message ("CMAKE Host :  " "Windows")
elseif(CMAKE_HOST_UNIX)
   set(LINUX 1)
   message ("CMAKE Host :  " "Linux")
endif()

message ("CMAKE_SYSTEM_PROCESSOR:" ${CMAKE_SYSTEM_PROCESSOR})
message ("CMAKE_LIB_SHARED:" ${CMAKE_LIB_SHARED})

if(NOT DEFINED ARCH)
	message("ARCH is not Set, default to x64")
	set(ARCH "x64")
endif()

if(WINDOWS)
   message ("ARCH is:" ${ARCH})
   set_property(GLOBAL PROPERTY USE_FOLDERS ON)
   add_definitions(-D_CRT_SECURE_NO_WARNINGS)
   set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
   set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /Zi")
   message ("cxx Flags:" ${CMAKE_CXX_FLAGS} /W4)
   message ("cxx Flags Debug:" ${CMAKE_CXX_FLAGS_Debug})
   message ("cxx Flags Release:" ${CMAKE_CXX_FLAGS_RELEASE})
   message ("CMAKE_GENERATOR_PLATFORM:" ${CMAKE_GENERATOR_PLATFORM})
endif()

if(LINUX)
   # if 32 bit then,
   # other way to check 32 bit 	if (CMAKE_CL_64) ?
   if((ARCH STREQUAL "x86") OR (${CMAKE_SYSTEM_PROCESSOR}  STREQUAL "i686"))
      message("ARCH is x86 & Building for 32 bit")
      set(CMAKE_C_FLAGS_DEBUG "-m32 -g")
      set(CMAKE_CXX_FLAGS_DEBUG "-m32 -g -std=gnu++11 -fPIC -Wno-write-strings")
      set(CMAKE_C_FLAGS_RELEASE "-m32 -O3 -DNDEBUG -s")
	  if((COVERAGE STREQUAL "TRUE"))
		  set(CMAKE_CXX_FLAGS_RELEASE "-m32 -g -Wno-write-strings -Wno-format -Wno-format-security -Wno-overflow -std=gnu++11 -fPIC -D_LINUX_ -DNDEBUG -s --coverage -fprofile-arcs -ftest-coverage")
	  else()
		  set(CMAKE_CXX_FLAGS_RELEASE "-m32 -g -Wno-write-strings -Wno-format -Wno-format-security -Wno-overflow -std=gnu++11 -fPIC -D_LINUX_ -DNDEBUG -s")
	  endif()
   else()
      # set C&C++ flags for debug and release
      message("ARCH is x64 & Building for 64 bit")
      set(CMAKE_C_FLAGS_DEBUG "-g")
      set(CMAKE_CXX_FLAGS_DEBUG "-g -std=gnu++11 -fPIC -D_LINUX_")
      set(CMAKE_C_FLAGS_RELEASE "-s -O3 -DNDEBUG")
	  if((COVERAGE STREQUAL "TRUE"))
		  set(CMAKE_CXX_FLAGS_RELEASE "-g -Wno-write-strings -Wno-format -Wno-format-security -Wno-overflow -std=gnu++11 -fPIC -D_LINUX_ -DNDEBUG -s --coverage -fprofile-arcs -ftest-coverage")
	  else()	
		  set(CMAKE_CXX_FLAGS_RELEASE "-g -Wno-write-strings -Wno-format -Wno-format-security -Wno-overflow -std=gnu++11 -fPIC -D_LINUX_ -DNDEBUG -s")
	  endif()	
   endif((ARCH STREQUAL "x86") OR (${CMAKE_SYSTEM_PROCESSOR}  STREQUAL "i686"))

   # Debug information
   message ("CMAKE_C_FLAGS_DEBUG:    " ${CMAKE_C_FLAGS_DEBUG})
   message ("CMAKE_CXX_FLAGS_DEBUG:  " ${CMAKE_CXX_FLAGS_DEBUG})
   message ("CMAKE_C_FLAGS_RELEASE:  " ${CMAKE_C_FLAGS_RELEASE})
   message ("CMAKE_CXX_FLAGS_RELEASE:" ${CMAKE_CXX_FLAGS_RELEASE})

endif(LINUX)


# build EDIE components
add_subdirectory(src/decoders)
add_subdirectory(src/hw_interface)

# build for google test runner
add_subdirectory(lib/googletest)
set_property(TARGET gtest PROPERTY FOLDER "googletest")
set_property(TARGET gtest_main PROPERTY FOLDER "googletest")
#add_subdirectory(lib/googletest)
add_subdirectory(src/decoders/common/test)
add_subdirectory(src/decoders/jsoninterface/test)
add_subdirectory(src/decoders/novatel/test)
add_subdirectory(src/hw_interface/stream_interface/test)

if(WINDOWS)
add_subdirectory(example/converter)
add_subdirectory(example/serialtest)
endif()

# This is an archive Static build(after combining stream, common, novatel and jsoninterface component into libEDIE.a) in Linux
# if we do not require it , We can comment this section.  
if(LINUX)
   add_custom_target(${PROJECT_NAME} ALL COMMAND ${CMAKE_AR} rc libEDIE.a $<TARGET_FILE:StreamInterface> $<TARGET_FILE:Common> $<TARGET_FILE:Novatel> $<TARGET_FILE:JsonInterface>)
install(FILES ${CMAKE_BINARY_DIR}/libEDIE.a DESTINATION lib)
endif()
