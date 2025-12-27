cmake_minimum_required(VERSION 3.28)
project(xattrplaying_plugin C)

find_library(VLC_PULSE_PATH NAMES vlc_pulse
        PATHS
            /usr/lib
            /usr/lib64
            /usr/local/lib
            /usr/local/lib64
        PATH_SUFFIXES "vlc"
        NO_CACHE REQUIRED)
get_filename_component(VLC_INSTALL_DIR "${VLC_PULSE_PATH}" DIRECTORY )
message(STATUS "VLC_INSTALL_DIR: ${VLC_INSTALL_DIR} from ${VLC_PULSE_PATH}")

# Set the source files for the extension
set(SOURCES
        library.c
)

# Find VLC libraries and headers
find_path(VLC_INCLUDE_DIR vlc.h PATH_SUFFIXES include/vlc REQUIRED)

message(STATUS "VLC_INCLUDE_DIR: ${VLC_INCLUDE_DIR}")

# Include directories for VLC headers
include_directories(
        ${VLC_INCLUDE_DIR}
        ${VLC_INCLUDE_DIR}/plugins
)

add_definitions(-DMODULE_STRING=\"xattrplaying_plugin\")
add_definitions(-D__PLUGIN__)
add_definitions(-D_REENTRANT)
add_definitions(-D_THREAD_SAFE)
add_definitions(-D_FILE_OFFSET_BITS=64)
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

# Specify the output shared library name
add_library(xattrplaying_plugin MODULE ${SOURCES})

# Link against VLC libraries
target_link_libraries(xattrplaying_plugin ${VLC_LIBRARY})

# Set the output directory for the shared library
set_target_properties(xattrplaying_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

## Set installation directory for the shared library
#install(TARGETS xattrplaying_plugin LIBRARY DESTINATION ${VLC_INSTALL_DIR}/plugins/misc/)
#
## Optionally: Copy the plugin to VLC plugins directory for development/testing
#add_custom_command(TARGET xattrplaying_plugin POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:xattrplaying_plugin> ${VLC_INSTALL_DIR}/plugins/$<TARGET_FILE_NAME:xattrplaying_plugin>
#)
