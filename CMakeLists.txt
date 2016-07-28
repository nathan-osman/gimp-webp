# gimp-webp - WebP Plugin for the GIMP
# Copyright (C) 2016  Nathan Osman & Ben Touchette
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

cmake_minimum_required(VERSION 2.8.12)
project(gimp-webp C)

# Set project version
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 4)
set(PROJECT_VERSION_PATCH 0)

find_package(PkgConfig REQUIRED)

# Because Gimp 2.8.x and 2.9.x are so vastly different, they must be handled
# differently - check the version of Gimp installed on the system. Also libwebp
# has changed significantly between versions, so the features that are compiled
# into the plugin depend on what's available there.

pkg_check_modules(GIMP REQUIRED
    gimp-2.0>=2.8
)

# Indicate which version was found
message(STATUS "Gimp ${GIMP_gimp-2.0_VERSION} found")

# Determine which set of packages to look for depending on version
if(GIMP_gimp-2.0_VERSION VERSION_LESS "2.9")
    pkg_check_modules(GIMP REQUIRED
        gimp-2.0>=2.8
        gimpui-2.0>=2.8
        libwebp>=0.4
        libwebpmux>=0.4
    )
else()
    pkg_check_modules(GIMP2 REQUIRED
        gimp-2.0>=2.9
        gimpui-2.0>=2.9
        libwebp>=0.5.1
        libwebpmux>=0.5.1
        libwebpdemux>=0.5.1
    )
endif()

# Add the include and library directories to the project
include_directories(${GIMP_INCLUDE_DIRS})
link_directories(${GIMP_LIBRARY_DIRS})

# Ensure that files in ${CMAKE_CURRENT_BINARY_DIR} are available
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Include the source directory
add_subdirectory(src)
