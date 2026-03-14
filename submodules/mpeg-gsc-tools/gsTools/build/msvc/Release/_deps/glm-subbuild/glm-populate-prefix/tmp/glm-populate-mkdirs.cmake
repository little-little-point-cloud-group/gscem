# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/dependencies/glm"
  "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/_deps/glm-build"
  "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/_deps/glm-subbuild/glm-populate-prefix"
  "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/_deps/glm-subbuild/glm-populate-prefix/tmp"
  "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/_deps/glm-subbuild/glm-populate-prefix/src/glm-populate-stamp"
  "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/_deps/glm-subbuild/glm-populate-prefix/src"
  "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/_deps/glm-subbuild/glm-populate-prefix/src/glm-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/_deps/glm-subbuild/glm-populate-prefix/src/glm-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/media/hipeson/21bd72c3-b2ba-406a-a594-1a7e99218c8a/hipeson/mmc_pcc/gly/gscem_all/submodules/mpeg-gsc-tools/gsTools/build/msvc/Release/_deps/glm-subbuild/glm-populate-prefix/src/glm-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
