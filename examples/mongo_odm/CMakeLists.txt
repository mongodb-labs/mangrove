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

include_directories(
    ${LIBBSON_INCLUDE_DIRS}
    ${LIBMONGOC_INCLUDE_DIRS}
    ${LIBBSONCXX_INCLUDE_DIRS}
    ${LIBMONGOCXX_INCLUDE_DIRS}
    ${CMAKE_INSTALL_PREFIX}/${MONGO_ODM_HEADER_INSTALL_DIR}
    ${CMAKE_INSTALL_PREFIX}/${BSON_MAPPER_HEADER_INSTALL_DIR}
    ${CMAKE_INSTALL_PREFIX}/include/
    ${CMAKE_INSTALL_PREFIX}/${BSON_MAPPER_HEADER_INSTALL_DIR}/bson_mapper/third_party
)

link_directories(
    ${LIBBSONCXX_LIBRARY_DIRS}
    ${LIBMONGOCXX_LIBRARY_DIRS}
    ${LIBBSON_LIBRARY_DIRS}
    ${LIBMONGOC_LIBRARY_DIRS}

    ${CMAKE_INSTALL_PREFIX}/lib/
)

set(MONGO_ODM_EXAMPLES
    demo.cpp
)

foreach(EXAMPLE_SRC ${MONGO_ODM_EXAMPLES})
    get_filename_component(EXAMPLE_TARGET ${EXAMPLE_SRC} NAME_WE)
    add_executable(${EXAMPLE_TARGET} ${EXAMPLE_SRC})
    target_link_libraries(${EXAMPLE_TARGET} mongo_odm bson_mapper mongocxx bsoncxx)
    set(MONGO_ODM_EXAMPLE_EXECUTABLES ${MONGO_ODM_EXAMPLE_EXECUTABLES} ${EXAMPLE_TARGET})
endforeach(EXAMPLE_SRC)

set(MONGO_ODM_EXAMPLE_EXECUTABLES ${MONGO_ODM_EXAMPLE_EXECUTABLES} PARENT_SCOPE)
