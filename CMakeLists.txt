cmake_minimum_required(VERSION 3.28)
project(xattrplaying_plugin C)

find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
    pkg_check_modules(VLC_PLUGIN QUIET vlc-plugin)
endif()

set(_VLC_CANDIDATE_LIB_DIRS
        /usr/lib/x86_64-linux-gnu
        /usr/lib64
        /usr/lib
        /usr/local/lib
        /usr/local/lib64)

if (VLC_PLUGIN_FOUND)
    message(STATUS "Found VLC using pkg-config")
    set(VLC_INCLUDE_DIRS
            ${VLC_PLUGIN_INCLUDE_DIRS}
            ${VLC_PLUGIN_INCLUDE_DIRS}/plugins)
    set(VLC_LIBRARIES ${VLC_PLUGIN_LIBRARIES})
else()
    find_path(VLC_INCLUDE_DIR
            NAMES vlc_common.h
            PATH_SUFFIXES vlc include/vlc
            REQUIRED)

    find_library(VLC_LIBRARIES
            NAMES vlccore
            PATHS ${_VLC_CANDIDATE_LIB_DIRS}
            PATH_SUFFIXES "" "vlc"
            REQUIRED)

    set(VLC_INCLUDE_DIRS
            ${VLC_INCLUDE_DIR}
            ${VLC_INCLUDE_DIR}/plugins)
endif()

# Set the source files for the extension
set(SOURCES
        library.c
)

# Include directories for VLC headers
include_directories(${VLC_INCLUDE_DIRS})

add_definitions(-DMODULE_STRING=\"xattrplaystat\")
add_definitions(-D__PLUGIN__)
add_definitions(-D_REENTRANT)
add_definitions(-D_THREAD_SAFE)
add_definitions(-D_FILE_OFFSET_BITS=64)
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

# Specify the output shared library name
add_library(xattrplaying_plugin MODULE ${SOURCES})

# Link against VLC libraries
target_link_libraries(xattrplaying_plugin ${VLC_LIBRARIES})

# Set the output directory for the shared library
set_target_properties(xattrplaying_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

## Set installation directory for the shared library
#install(TARGETS xattrplaying_plugin LIBRARY DESTINATION ${VLC_INSTALL_DIR}/plugins/misc/)
#
## Optionally: Copy the plugin to VLC plugins directory for development/testing
#add_custom_command(TARGET xattrplaying_plugin POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:xattrplaying_plugin> ${VLC_INSTALL_DIR}/plugins/$<TARGET_FILE_NAME:xattrplaying_plugin>
#)
