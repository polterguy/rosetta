################################################################################
#
# Rosetta web server, copyright(c) 2016, Thomas Hansen, phosphorusfive@gmail.com.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License, as published by
# the Free Software Foundation, version 3.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# CMake file for the Rosetta web server.
#
################################################################################

cmake_minimum_required (VERSION 2.8)

# Project name
project (rosetta)

# Making sure "root directory" for code is root for inclusion of header files
include_directories(.)

# Settings compiler flags
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y -Wall -Wextra -Wno-unused-parameter")

# Making sure output is stuffed into correct directory
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})

# Including boost libraries
find_package (Boost COMPONENTS filesystem thread REQUIRED)

# Making sure we include OpenSSL libraries.
if(APPLE)
    set(OPENSSL_ROOT_DIR "/usr/local/opt/openssl")
endif()
find_package(OpenSSL REQUIRED)
if (OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

# Adding source files to compilation of main Rosetta project.
file (GLOB MAIN "main.cpp")
file (GLOB_RECURSE HTTP_SERVER "http_server/src/*.cpp")

# Adding all C++ files compiler should compile into main executable
add_executable (rosetta ${MAIN} ${HTTP_SERVER})

# Creating "configuration" library as a statically linked library
file (GLOB COMMON "common/src/*.cpp")
add_library (rosetta_common STATIC ${COMMON})

# Making sure we link to boost during linking process, in addition to all additionally built libraries,
# such as "configuration" library
target_link_libraries (rosetta ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} rosetta_common)



