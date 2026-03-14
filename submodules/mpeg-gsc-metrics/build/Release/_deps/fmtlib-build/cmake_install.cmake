# Install script for directory: /media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/lib/libfmt.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/fmt" TYPE FILE FILES
    "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-src/include/fmt/args.h"
    "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-src/include/fmt/chrono.h"
    "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-src/include/fmt/color.h"
    "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-src/include/fmt/compile.h"
    "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-src/include/fmt/core.h"
    "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-src/include/fmt/format.h"
    "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-src/include/fmt/format-inl.h"
    "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-src/include/fmt/os.h"
    "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-src/include/fmt/ostream.h"
    "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-src/include/fmt/printf.h"
    "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-src/include/fmt/ranges.h"
    "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-src/include/fmt/std.h"
    "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-src/include/fmt/xchar.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt" TYPE FILE FILES
    "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-build/fmt-config.cmake"
    "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-build/fmt-config-version.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt/fmt-targets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt/fmt-targets.cmake"
         "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-build/CMakeFiles/Export/b834597d9b1628ff12ae4314c3a2e4b8/fmt-targets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt/fmt-targets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt/fmt-targets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt" TYPE FILE FILES "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-build/CMakeFiles/Export/b834597d9b1628ff12ae4314c3a2e4b8/fmt-targets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/fmt" TYPE FILE FILES "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-build/CMakeFiles/Export/b834597d9b1628ff12ae4314c3a2e4b8/fmt-targets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-metrics/build/Release/_deps/fmtlib-build/fmt.pc")
endif()

