# Install script for directory: /mnt/d/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nbis/nbis/openjp2/src/lib/openjp2

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
    set(CMAKE_INSTALL_CONFIG_NAME "")
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
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/openjpeg-2.1/OpenJPEGTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/openjpeg-2.1/OpenJPEGTargets.cmake"
         "/mnt/d/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nbis/nbis/openjp2/src/lib/openjp2/build/CMakeFiles/Export/9576e7155085235c3ad6342b2ec432ed/OpenJPEGTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/openjpeg-2.1/OpenJPEGTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/openjpeg-2.1/OpenJPEGTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/openjpeg-2.1" TYPE FILE FILES "/mnt/d/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nbis/nbis/openjp2/src/lib/openjp2/build/CMakeFiles/Export/9576e7155085235c3ad6342b2ec432ed/OpenJPEGTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^()$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/openjpeg-2.1" TYPE FILE FILES "/mnt/d/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nbis/nbis/openjp2/src/lib/openjp2/build/CMakeFiles/Export/9576e7155085235c3ad6342b2ec432ed/OpenJPEGTargets-noconfig.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/openjpeg-2.1" TYPE FILE FILES "/mnt/d/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nbis/nbis/openjp2/src/lib/openjp2/build/OpenJPEGConfig.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/mnt/d/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nbis/nbis/openjp2/src/lib/openjp2/build/libopenjp2.pc")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/mnt/d/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nbis/nbis/openjp2/src/lib/openjp2/build/src/lib/cmake_install.cmake")
  include("/mnt/d/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nbis/nbis/openjp2/src/lib/openjp2/build/thirdparty/cmake_install.cmake")
  include("/mnt/d/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nbis/nbis/openjp2/src/lib/openjp2/build/src/bin/cmake_install.cmake")
  include("/mnt/d/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nbis/nbis/openjp2/src/lib/openjp2/build/wrapping/cmake_install.cmake")

endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/mnt/d/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nbis/nbis/openjp2/src/lib/openjp2/build/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
