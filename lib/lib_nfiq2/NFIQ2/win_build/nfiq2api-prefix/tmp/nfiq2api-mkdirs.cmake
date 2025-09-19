# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "D:/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nfiq2/NFIQ2/NFIQ2/NFIQ2Api")
  file(MAKE_DIRECTORY "D:/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nfiq2/NFIQ2/NFIQ2/NFIQ2Api")
endif()
file(MAKE_DIRECTORY
  "D:/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nfiq2/NFIQ2/win_build/nfiq2api-prefix/src/nfiq2api-build"
  "D:/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nfiq2/NFIQ2/win_build/nfiq2api-prefix"
  "D:/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nfiq2/NFIQ2/win_build/nfiq2api-prefix/tmp"
  "D:/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nfiq2/NFIQ2/win_build/nfiq2api-prefix/src/nfiq2api-stamp"
  "D:/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nfiq2/NFIQ2/win_build/nfiq2api-prefix/src"
  "D:/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nfiq2/NFIQ2/win_build/nfiq2api-prefix/src/nfiq2api-stamp"
)

set(configSubDirs Release)
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nfiq2/NFIQ2/win_build/nfiq2api-prefix/src/nfiq2api-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nfiq2/NFIQ2/win_build/nfiq2api-prefix/src/nfiq2api-stamp${cfgdir}") # cfgdir has leading slash
endif()
