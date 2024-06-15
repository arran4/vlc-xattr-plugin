cmake_minimum_required(VERSION 3.28)
project(vlc_xattr_playing C)

# Set VLC installation directory (default for Gentoo)
set(VLC_INSTALL_DIR "/usr" CACHE PATH "VLC installation directory")

# Set the source files for the extension
set(SOURCES
        library.c
)

# Find VLC libraries and headers
find_path(VLC_INCLUDE_DIR vlc.h)
find_library(VLC_LIBRARY NAMES vlc)

# Include directories for VLC headers
include_directories(
        ${VLC_INCLUDE_DIR}/vlc
        ${VLC_INCLUDE_DIR}/vlc/plugins
)

# Specify the output shared library name
add_library(vlc_xattr_playing MODULE ${SOURCES})

# Link against VLC libraries
target_link_libraries(vlc_xattr_playing ${VLC_LIBRARY})

# Set the output directory for the shared library
set_target_properties(vlc_xattr_playing PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

# Set installation directory for the shared library
install(TARGETS vlc_xattr_playing LIBRARY DESTINATION ${VLC_INSTALL_DIR}/lib/vlc/plugins/)

# Optionally: Copy the plugin to VLC plugins directory for development/testing
add_custom_command(TARGET vlc_xattr_playing POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:vlc_xattr_playing> ${VLC_INSTALL_DIR}/lib/vlc/plugins/$<TARGET_FILE_NAME:vlc_xattr_playing>
)
