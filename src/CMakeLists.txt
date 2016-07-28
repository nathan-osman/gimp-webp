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

# Prepare the configuration file
configure_file(config.h.in "${CMAKE_CURRENT_BINARY_DIR}/config.h")

# Specify each of the required source files
set(SRC
    webp-dialog.c
    webp-load.c
    webp-save.c
    webp.c)

# Build the file-webp executable
add_executable(file-webp ${SRC})
target_link_libraries(file-webp ${GIMP_LIBRARIES})

# In order to determine the correct installation directory, we need to directly
# invoke pkg-config to obtain the lib/ directory
execute_process(COMMAND
    ${PKG_CONFIG_EXECUTABLE} --variable=gimplibdir gimp-2.0
    RESULT_VARIABLE PKG_CONFIG_RESULT
    OUTPUT_VARIABLE GIMP_LIB_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# Only set an installation directory if the command succeeded
if(PKG_CONFIG_RESULT)
    message(WARNING "gimplibdir not set, install target unavailable")
else()
    install(TARGETS file-webp RUNTIME DESTINATION "${GIMP_LIB_DIR}/plug-ins")
endif()
