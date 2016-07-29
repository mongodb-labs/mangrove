# Copyright 2016 MongoDB Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

project(BOSON)

set(LIBBSONCXX_REQUIRED_VERSION 3.0.1)
set(LIBBSONCXX_REQUIRED_ABI_VERSION v_noabi)
find_package(libbsoncxx ${LIBBSONCXX_REQUIRED_VERSION} REQUIRED)

set(LIBMONGOCXX_REQUIRED_VERSION 3.0.1)
set(LIBMONGOCXX_REQUIRED_ABI_VERSION v_noabi)
find_package(libmongocxx ${LIBMONGOCXX_REQUIRED_VERSION} REQUIRED)

set(LIBBSON_REQUIRED_VERSION 1.3.4)
set(LIBBSON_REQUIRED_ABI_VERSION 1.0)
find_package(LibBSON ${LIBBSON_REQUIRED_VERSION} REQUIRED)

set(LIBMONGOC_REQUIRED_VERSION 1.3.4)
set(LIBMONGOC_REQUIRED_ABI_VERSION 1.0)
find_package(LibMongoC ${LIBMONGOC_REQUIRED_VERSION} REQUIRED)

# Update these as needed.
# TODO: read from file
set(BOSON_VERSION_MAJOR 0)
set(BOSON_VERSION_MINOR 0)
set(BOSON_VERSION_PATCH 1)
set(BOSON_VERSION_EXTRA "-alpha")
set(BOSON_ABI_VERSION _noabi)

set(BOSON_VERSION ${BOSON_VERSION_MAJOR}.${BOSON_VERSION_MINOR}.${BOSON_VERSION_PATCH}${BOSON_VERSION_EXTRA})
set(BOSON_INLINE_NAMESPACE "v${BOSON_ABI_VERSION}")
set(BOSON_HEADER_INSTALL_DIR "include/boson/${BOSON_INLINE_NAMESPACE}" CACHE INTERNAL "")

add_subdirectory(config)

set(boson_sources
   "bson_streambuf.cpp"
)

include_directories(
    ${LIBBSONCXX_INCLUDE_DIRS}
    ${LIBMONGOCXX_INCLUDE_DIRS}
    ${LIBBSON_INCLUDE_DIRS}
    ${LIBMONGOC_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_BINARY_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/third_party/cereal/include
)

link_directories(
    ${LIBBSONCXX_LIBRARY_DIRS}
    ${LIBMONGOCXX_LIBRARY_DIRS}
    ${LIBBSON_LIBRARY_DIRS}
    ${LIBMONGOC_LIBRARY_DIRS}
)

add_library(boson_static STATIC
    ${boson_sources}
)

target_compile_definitions(boson_static PUBLIC BOSON_STATIC)

set_target_properties(boson_static PROPERTIES
    OUTPUT_NAME boson
)

# Follow the boost convention to disambiguate the dll and static
# library names
if (WIN32)
    set_target_properties(boson_static PROPERTIES
        PREFIX lib
    )
endif()

add_library(boson SHARED
    ${boson_sources}
)

set_target_properties (boson PROPERTIES
    OUTPUT_NAME boson
    VERSION ${BOSON_VERSION}
    DEFINE_SYMBOL BOSON_EXPORTS
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    SOVERSION ${BOSON_ABI_VERSION}
)

generate_export_header (boson
    BASE_NAME BOSON
    EXPORT_MACRO_NAME BOSON_API
    NO_EXPORT_MACRO_NAME BOSON_PRIVATE
    EXPORT_FILE_NAME config/export.hpp
    STATIC_DEFINE BOSON_STATIC
)

set(boson_libs ${LIBBSONCXX_LIBRARIES} ${LIBMONGOCXX_LIBRARIES} ${LIBMONGOC_LIBRARIES} ${LIBBSON_LIBRARIES})

target_link_libraries(boson_static ${boson_libs})
target_link_libraries(boson PRIVATE ${boson_libs})

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DESTINATION ${BOSON_HEADER_INSTALL_DIR}
    COMPONENT dev
    FILES_MATCHING
        PATTERN "*.h"
        PATTERN "*.hpp"
        REGEX ".+/test" EXCLUDE
)



install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/third_party/cereal/include/cereal/
    DESTINATION ${BOSON_HEADER_INSTALL_DIR}/boson/third_party/cereal
    FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.h"
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/config/export.hpp
    DESTINATION ${BOSON_HEADER_INSTALL_DIR}/boson/config
    COMPONENT dev
)

install(TARGETS
    boson
    RUNTIME DESTINATION bin COMPONENT runtime
    LIBRARY DESTINATION lib COMPONENT runtime
    ARCHIVE DESTINATION lib COMPONENT dev
)

install(TARGETS
    boson_static
    ARCHIVE DESTINATION lib COMPONENT dev
)

set(PACKAGE_INCLUDE_INSTALL_DIRS ${BOSON_HEADER_INSTALL_DIR})
set(PACKAGE_LIBRARY_INSTALL_DIRS lib)
set(PACKAGE_LIBRARIES boson)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  cmake/libboson-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/libboson-config.cmake
  INSTALL_DESTINATION lib/cmake/libboson-${BOSON_VERSION}
  PATH_VARS PACKAGE_INCLUDE_INSTALL_DIRS PACKAGE_LIBRARY_INSTALL_DIRS
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/libboson-config-version.cmake
  VERSION ${BOSON_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/libboson-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/libboson-config-version.cmake
  DESTINATION lib/cmake/libboson-${BOSON_VERSION}
)

add_subdirectory(test)
