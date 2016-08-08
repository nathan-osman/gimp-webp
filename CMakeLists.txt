# gimp-webp - WebP Plugin for the GIMP
# Copyright (C) 2015  Nathan Osman & Ben Touchette
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 2.8.18)
project(gimp-webp C)

# Set project version
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 4)
set(PROJECT_VERSION_PATCH 1)

find_package(PkgConfig REQUIRED)

# Find the packages that contain the Gimp headers and libraries
pkg_check_modules(GIMP REQUIRED
    gimp-2.0>=2.8
    gimpui-2.0>=2.8
)

include_directories(${GIMP_INCLUDE_DIRS})
link_directories(${GIMP_LIBRARY_DIRS})

if("${GIMP_gimp-2.0_VERSION}" VERSION_LESS "2.9")
    set(GIMP_2_9 0)
else()
    set(GIMP_2_9 1)

    # GEGL is required for GIMP 2.9
    pkg_check_modules(GEGL REQUIRED
        gegl-0.3>=0.3.8
    )

    message(STATUS "GEGL ${GEGL_gegl-0.3_VERSION} found")

    include_directories(${GEGL_INCLUDE_DIRS})
    link_directories(${GEGL_LIBRARY_DIRS})
endif()

# Find the packages that contain the WebP headers and libraries
pkg_check_modules(WEBP REQUIRED
    libwebp>=0.5
    libwebpmux>=0.5
    libwebpdemux>=0.5)

message(STATUS "WebP ${WEBP_libwebp_VERSION} found")

include_directories(${WEBP_INCLUDE_DIRS})
link_directories(${WEBP_LIBRARY_DIRS})

# Ensure that files in ${CMAKE_CURRENT_BINARY_DIR} are available
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Include the source directory
add_subdirectory(src)
