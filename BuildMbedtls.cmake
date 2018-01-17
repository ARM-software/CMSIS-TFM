#-------------------------------------------------------------------------------
# Copyright (c) 2017, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

#When included, this file will add a target to build the mbedtls libraries with
#the same compilation setting as used by the file including this one.
cmake_minimum_required(VERSION 3.7)

#Define where mbedtls intermediate output files are stored.
set (MBEDTLS_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/mbedtls")

#Check input variables
if(NOT DEFINED MBEDTLS_BUILD_TYPE)
	message(FATAL_ERROR "Please set MBEDTLS_BUILD_TYPE to 'Debug' or 'Release' before including this file.")
endif()

if(NOT DEFINED MBEDTLS_SOURCE_DIR)
	message(FATAL_ERROR "Please set MBEDTLS_SOURCE_DIR before including this file.")
endif()

if(NOT DEFINED MBEDTLS_INSTALL_DIR)
	message(FATAL_ERROR "Please set MBEDTLS_INSTALL_DIR before including this file.")
endif()

if(NOT DEFINED MBEDTLS_C_FLAGS)
	message(FATAL_ERROR "Please set MBEDTLS_C_FLAGS before including this file.")
endif()

if(NOT DEFINED MBEDTLS_TARGET_NAME)
	message(FATAL_ERROR "Please set MBEDTLS_TARGET_NAME before including this file.")
endif()

string(APPEND MBEDTLS_C_FLAGS ${CMAKE_C_FLAGS})

if (TARGET ${MBEDTLS_TARGET_NAME})
	message(FATAL_ERROR "A target with name ${MBEDTLS_TARGET_NAME} is already\
defined. Please set MBEDTLS_TARGET_NAME to a unique value.")
endif()

#Build mbedtls as external project.
#This ensures mbedtls is built with exactly defined settings.
#mbedtls will be used from is't install location
include(ExternalProject)
# Add mbed TLS files to the build.
set(_static_lib_command ${CMAKE_C_CREATE_STATIC_LIBRARY})
externalproject_add(${MBEDTLS_TARGET_NAME}
	SOURCE_DIR ${MBEDTLS_SOURCE_DIR}
	#Set mbedtls features
	CMAKE_ARGS -DENABLE_TESTING=OFF -DENABLE_PROGRAMS=OFF
	#Enforce our build system's settings.
	CMAKE_ARGS -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH} -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
	#Inherit the build setting of this project
	CMAKE_ARGS -DCMAKE_BUILD_TYPE=${MBEDTLS_BUILD_TYPE}
	#C compiler settings
	CMAKE_CACHE_ARGS -DCMAKE_C_COMPILER:string=${CMAKE_C_COMPILER}
	CMAKE_CACHE_ARGS -DCMAKE_C_COMPILER_ID:string=${CMAKE_C_COMPILER_ID}
	CMAKE_CACHE_ARGS -DCMAKE_C_FLAGS:string=${MBEDTLS_C_FLAGS}
	CMAKE_CACHE_ARGS -DCMAKE_C_FLAGS_DEBUG:string=${CMAKE_C_FLAGS_DEBUG}
	CMAKE_CACHE_ARGS -DCMAKE_C_FLAGS_RELEASE:string=${CMAKE_C_FLAGS_RELEASE}
	CMAKE_CACHE_ARGS -DCMAKE_C_OUTPUT_EXTENSION:string=.o
	CMAKE_CACHE_ARGS -DCMAKE_C_COMPILER_WORKS:bool=true
	#Archiver settings
	CMAKE_CACHE_ARGS -DCMAKE_AR:string=${CMAKE_AR}
	CMAKE_CACHE_ARGS -DCMAKE_C_CREATE_STATIC_LIBRARY:internal=${_static_lib_command}
	CMAKE_CACHE_ARGS -DCMAKE_C_LINK_EXECUTABLE:string=${CMAKE_C_LINK_EXECUTABLE}
	CMAKE_CACHE_ARGS -DCMAKE_STATIC_LIBRARY_PREFIX_C:string=${CMAKE_STATIC_LIBRARY_PREFIX_C}
	CMAKE_CACHE_ARGS -DCMAKE_STATIC_LIBRARY_PREFIX_CXX:string=${CMAKE_STATIC_LIBRARY_PREFIX_CXX}
	#Install location
	CMAKE_CACHE_ARGS -DCMAKE_INSTALL_PREFIX:string=${MBEDTLS_INSTALL_DIR}
	#Place for intermediate build files
	BINARY_DIR ${MBEDTLS_BINARY_DIR})

#Add an install target to force installation after each mbedtls build. Without
#this target installation happens only when a clean mbedtls build is executed.
add_custom_target(${MBEDTLS_TARGET_NAME}_install
	 COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR}/mbedtls -- install
	 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/mbedtls
	 COMMENT "Installing mbedtls to ${MBEDTLS_INSTALL_DIR}"
	 VERBATIM)
#Make install rule depend on mbedtls library build
add_dependencies(${MBEDTLS_TARGET_NAME}_install ${MBEDTLS_TARGET_NAME})
