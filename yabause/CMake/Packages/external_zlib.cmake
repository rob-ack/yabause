# Copyright 2017 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================
include (ExternalProject)

set(zlib_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/zlib/install/include)
set(ZLIB_URL https://github.com/madler/zlib)
set(ZLIB_BUILD ${CMAKE_CURRENT_BINARY_DIR}/zlib/src/zlib)
set(ZLIB_INSTALL ${CMAKE_CURRENT_BINARY_DIR}/zlib/install)
set(ZLIB_TAG 9f0f2d4f9f1f28be7e16d8bf3b4e9d4ada70aa9f)

if(WIN32)
  set(zlib_STATIC_LIBRARIES
      debug ${CMAKE_CURRENT_BINARY_DIR}/zlib/install/lib/zlibstaticd.lib
      optimized ${CMAKE_CURRENT_BINARY_DIR}/zlib/install/lib/zlibstatic.lib)
else()
  set(zlib_STATIC_LIBRARIES
      ${CMAKE_CURRENT_BINARY_DIR}/zlib/install/lib/libz.a)
endif()

if(ANDROID)
get_filename_component(TOOL_CHAIN_ABSOLUTE_PATH "${CMAKE_TOOLCHAIN_FILE}"
                       REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")

set( ADDITIONAL_CMAKE_ARGS 
 -DANDROID_ABI=${ANDROID_ABI}
 -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM} 
 -DANDROID_NATIVE_API_LEVEL=${ANDROID_NATIVE_API_LEVEL}
 -DCMAKE_TOOLCHAIN_FILE=${TOOL_CHAIN_ABSOLUTE_PATH}
)

elseif(IOS)

  get_filename_component(TOOL_CHAIN_ABSOLUTE_PATH "${CMAKE_TOOLCHAIN_FILE}"
                       REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
  set( ADDITIONAL_CMAKE_ARGS 
    -DCMAKE_TOOLCHAIN_FILE=${TOOL_CHAIN_ABSOLUTE_PATH}
    -DPLATFORM=${PLATFORM}
  )

else()
  set(ADDITIONAL_CMAKE_ARGS "")
endif()

set(ZLIB_HEADERS
    "${ZLIB_INSTALL}/include/zconf.h"
    "${ZLIB_INSTALL}/include/zlib.h"
)

ExternalProject_Add(
    zlib
    PREFIX zlib
    GIT_REPOSITORY ${ZLIB_URL}
    GIT_TAG ${ZLIB_TAG}
    INSTALL_DIR ${ZLIB_INSTALL}
    BUILD_IN_SOURCE 1
    DOWNLOAD_DIR "${DOWNLOAD_LOCATION}"
    CMAKE_ARGS
    -DZLIB_BUILD_EXAMPLES=OFF
		-DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=ON
        -DCMAKE_BUILD_TYPE:STRING=Release
        -DCMAKE_INSTALL_PREFIX:STRING=${ZLIB_INSTALL}
        ${ADDITIONAL_CMAKE_ARGS}        
)

set( LIBZ_LIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/zlib/install/lib/ )

# put zlib includes in the directory where they are expected
add_custom_target(zlib_create_destination_dir
    COMMAND ${CMAKE_COMMAND} -E make_directory ${zlib_INCLUDE_DIR}
    DEPENDS zlib)

add_custom_target(zlib_copy_headers_to_destination
    DEPENDS zlib_create_destination_dir)

foreach(header_file ${ZLIB_HEADERS})
    add_custom_command(TARGET zlib_copy_headers_to_destination PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${header_file} ${zlib_INCLUDE_DIR})
endforeach()