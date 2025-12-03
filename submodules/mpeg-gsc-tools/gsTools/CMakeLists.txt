cmake_minimum_required (VERSION 3.13.2)

get_filename_component(MYNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" MYNAME ${MYNAME})
project(${MYNAME} C CXX)

# Options
include( CheckCXXCompilerFlag )

# Building options
set(CMAKE_CXX_STANDARD              20)
set(CMAKE_CXX_STANDARD_REQUIRED     OFF)
set(CMAKE_CXX_EXTENSIONS            OFF) 
set(CMAKE_EXPORT_COMPILE_COMMANDS   ON)
set(CMAKE_CXX_COMPILER_WORKS        1)
SET(CMAKE_CONFIGURATION_TYPES       Release CACHE STRING "" FORCE )
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
ADD_DEFINITIONS(" -DCMAKE_BUILD_TYPE=${CMAKE_CONFIGURATION_TYPES} ")
if (MSVC)
  if (CMAKE_CXX_COMPILER_ID MATCHES "intel")
    add_definitions("/W3 /D_CRT_SECURE_NO_WARNINGS /std:c++20")
  else()
    add_definitions("/W3 /D_CRT_SECURE_NO_WARNINGS /Oy /nologo /std:c++20 ")  
  endif()
else ()
  add_compile_options(-Wall 
                      -Wextra
                      -Wno-unused-function  
                      -Wno-unused-parameter 
                      -Wno-deprecated-declarations)
endif()

if (NOT WIN32)
  message("Unix: CMAKE_SYSTEM_PROCESSOR = ${CMAKE_SYSTEM_PROCESSOR} CMAKE_CXX_COMPILER_ID  = ${CMAKE_CXX_COMPILER_ID}")
  if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    message("ARM compiler ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=armv8-a+simd  -DNDEBUG") 
  else()
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      message("Clang compiler ")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mfpmath=sse -msse4.2 -m64 -DNDEBUG -lstdc++ " )
    else()
      message("GCC compiler ")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mfpmath=sse -msse4.2 -m64 -DNDEBUG -lstdc++ ") 
      # link_libraries(stdc++fs)
    endif()
  endif()
else()
  message("Win32 CMAKE_CXX_COMPILER_ID = ${CMAKE_CXX_COMPILER_ID}")
  add_definitions(-D_USE_MATH_DEFINES)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message("Clang compiler ")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mfpmath=sse -msse4.2 -m64 -DNDEBUG " ) 
  else()
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
      message("GCC compiler ")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mfpmath=sse -msse4.2 -m64 -DNDEBUG -static -lstdc++") 
    endif()
  endif()
endif() 

# Dependencies
include( ${CMAKE_SOURCE_DIR}/dependencies/cmake/tools.cmake )
include( ${CMAKE_SOURCE_DIR}/dependencies/cmake/clang-format.cmake )
include( ${CMAKE_SOURCE_DIR}/dependencies/cmake/glm.cmake )

# Set files and include lists
FILE( GLOB SRC ${CMAKE_SOURCE_DIR}/source/util/*.cpp ${CMAKE_SOURCE_DIR}/dependencies/program-options-lite/program_options_lite.cpp)

set( INCLUDE_LIST
     ${CMAKE_SOURCE_DIR}/source
     ${CMAKE_SOURCE_DIR}/dependencies
     ${CMAKE_SOURCE_DIR}/dependencies/glm
     ${CMAKE_SOURCE_DIR}/dependencies/program-options-lite
)

# Applications
create_executable(NAME      cameraPosition 
                  SOURCES   ${CMAKE_SOURCE_DIR}/source/addCameraPosition.cpp 
                            ${CMAKE_SOURCE_DIR}/dependencies/program-options-lite/program_options_lite.cpp
                  INCLUDES  ${INCLUDE_LIST} )  

                




                          