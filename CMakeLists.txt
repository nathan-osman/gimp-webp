# gimp-webp - WebP Plugin for the GIMP
# Copyright (C) 2015  Nathan Osman
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
set(PROJECT_VERSION_MINOR 3)
set(PROJECT_VERSION_PATCH 0)

# Find the packages that contain the Gimp/WebP headers and libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(GIMP REQUIRED
    gimp-2.0>=2.8
    gimpui-2.0>=2.8
    libwebp>=0.4)

# Add the include and library directories to the project
include_directories(${GIMP_INCLUDE_DIRS})
link_directories(${GIMP_LIBRARY_DIRS})

# Include the source directory
add_subdirectory(src)
