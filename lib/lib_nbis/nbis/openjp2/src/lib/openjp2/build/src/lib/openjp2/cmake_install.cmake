# Install script for directory: /mnt/d/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nbis/nbis/openjp2/src/lib/openjp2/src/lib/openjp2

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

if(CMAKE_INSTALL_COMPONENT STREQUAL "Headers" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/openjpeg-2.1" TYPE FILE FILES "/mnt/d/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nbis/nbis/openjp2/src/lib/openjp2/build/src/lib/openjp2/opj_config.h")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Libraries" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/mnt/d/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nbis/nbis/openjp2/src/lib/openjp2/build/bin/libopenjp2.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Headers" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/openjpeg-2.1" TYPE FILE FILES
    "/mnt/d/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nbis/nbis/openjp2/src/lib/openjp2/src/lib/openjp2/openjpeg.h"
    "/mnt/d/SVN/PCTool/imd_fap50_sdk/trunk3/lib/lib_nbis/nbis/openjp2/src/lib/openjp2/src/lib/openjp2/opj_stdint.h"
    )
endif()

