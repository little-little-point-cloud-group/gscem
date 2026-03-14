# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

if(EXISTS "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/_deps/glm-subbuild/glm-populate-prefix/src/glm-populate-stamp/glm-populate-gitclone-lastrun.txt" AND EXISTS "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/_deps/glm-subbuild/glm-populate-prefix/src/glm-populate-stamp/glm-populate-gitinfo.txt" AND
  "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/_deps/glm-subbuild/glm-populate-prefix/src/glm-populate-stamp/glm-populate-gitclone-lastrun.txt" IS_NEWER_THAN "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/_deps/glm-subbuild/glm-populate-prefix/src/glm-populate-stamp/glm-populate-gitinfo.txt")
  message(STATUS
    "Avoiding repeated git clone, stamp file is up to date: "
    "'/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/_deps/glm-subbuild/glm-populate-prefix/src/glm-populate-stamp/glm-populate-gitclone-lastrun.txt'"
  )
  return()
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} -E rm -rf "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/dependencies/glm"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to remove directory: '/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/dependencies/glm'")
endif()

# try the clone 3 times in case there is an odd git clone issue
set(error_code 1)
set(number_of_tries 0)
while(error_code AND number_of_tries LESS 3)
  execute_process(
    COMMAND "/usr/bin/git" 
            clone --no-checkout --config "advice.detachedHead=false" "https://github.com/g-truc/glm.git" "glm"
    WORKING_DIRECTORY "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/dependencies"
    RESULT_VARIABLE error_code
  )
  math(EXPR number_of_tries "${number_of_tries} + 1")
endwhile()
if(number_of_tries GREATER 1)
  message(STATUS "Had to git clone more than once: ${number_of_tries} times.")
endif()
if(error_code)
  message(FATAL_ERROR "Failed to clone repository: 'https://github.com/g-truc/glm.git'")
endif()

execute_process(
  COMMAND "/usr/bin/git" 
          checkout "1.0.0" --
  WORKING_DIRECTORY "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/dependencies/glm"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to checkout tag: '1.0.0'")
endif()

set(init_submodules TRUE)
if(init_submodules)
  execute_process(
    COMMAND "/usr/bin/git" 
            submodule update --recursive --init 
    WORKING_DIRECTORY "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/dependencies/glm"
    RESULT_VARIABLE error_code
  )
endif()
if(error_code)
  message(FATAL_ERROR "Failed to update submodules in: '/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/dependencies/glm'")
endif()

# Complete success, update the script-last-run stamp file:
#
execute_process(
  COMMAND ${CMAKE_COMMAND} -E copy "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/_deps/glm-subbuild/glm-populate-prefix/src/glm-populate-stamp/glm-populate-gitinfo.txt" "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/_deps/glm-subbuild/glm-populate-prefix/src/glm-populate-stamp/glm-populate-gitclone-lastrun.txt"
  RESULT_VARIABLE error_code
)
if(error_code)
  message(FATAL_ERROR "Failed to copy script-last-run stamp file: '/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/_deps/glm-subbuild/glm-populate-prefix/src/glm-populate-stamp/glm-populate-gitclone-lastrun.txt'")
endif()
