cmake_minimum_required(VERSION 3.28)
project(xattrplaying_plugin C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(CTest)
include(GNUInstallDirs)

find_package(PkgConfig)
if(PkgConfig_FOUND)
    pkg_check_modules(VLC QUIET vlc-plugin)
    if(NOT VLC_FOUND)
        pkg_check_modules(VLC QUIET libvlc)
    endif()
    if(NOT VLC_FOUND)
        message(STATUS "VLC (vlc-plugin or libvlc) not found via pkg-config, falling back to default search paths")
    endif()
endif()

find_library(VLC_PULSE_PATH NAMES vlc_pulse
        PATHS
            /usr/lib
            /usr/lib64
            /usr/local/lib
            /usr/local/lib64
            ${CMAKE_INSTALL_LIBDIR}
            ${CMAKE_INSTALL_FULL_LIBDIR}
        PATH_SUFFIXES "vlc"
        NO_CACHE)
if(VLC_PULSE_PATH)
    get_filename_component(VLC_INSTALL_DIR "${VLC_PULSE_PATH}" DIRECTORY )
    message(STATUS "VLC_INSTALL_DIR: ${VLC_INSTALL_DIR} from ${VLC_PULSE_PATH}")
endif()

if(NOT VLC_INSTALL_DIR)
    set(VLC_INSTALL_DIR "${CMAKE_INSTALL_FULL_LIBDIR}/vlc")
endif()
set(VLC_PLUGIN_INSTALL_DIR "${VLC_INSTALL_DIR}/plugins/misc" CACHE PATH
        "Installation directory for the VLC misc plugin")

# Set the source files for the extension
set(SOURCES
        library.c
        tag_utils.c
)

# Find VLC libraries and headers
find_path(VLC_INCLUDE_DIR vlc_common.h
        PATHS ${VLC_INCLUDE_DIRS} /usr/include /usr/local/include
        PATH_SUFFIXES include/vlc vlc vlc/plugins plugins
)

find_library(VLC_LIBVLC_LIBRARY
        NAMES vlc libvlc
        HINTS ${VLC_LIBRARY_DIRS}
        PATHS /usr/lib /usr/lib64 /usr/local/lib /usr/local/lib64 ${CMAKE_INSTALL_LIBDIR} ${CMAKE_INSTALL_FULL_LIBDIR}
)

find_library(VLC_VLCCORE_LIBRARY
        NAMES vlccore libvlccore
        HINTS ${VLC_LIBRARY_DIRS}
        PATHS /usr/lib /usr/lib64 /usr/local/lib /usr/local/lib64 ${CMAKE_INSTALL_LIBDIR} ${CMAKE_INSTALL_FULL_LIBDIR}
)

if (VLC_INCLUDE_DIR AND VLC_LIBVLC_LIBRARY AND VLC_VLCCORE_LIBRARY)
    message(STATUS "VLC_INCLUDE_DIR: ${VLC_INCLUDE_DIR}")
    message(STATUS "VLC_LIBVLC_LIBRARY: ${VLC_LIBVLC_LIBRARY}")
    message(STATUS "VLC_VLCCORE_LIBRARY: ${VLC_VLCCORE_LIBRARY}")

    # Include directories for VLC headers
    include_directories(
            ${VLC_INCLUDE_DIR}
            ${VLC_INCLUDE_DIRS}
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
    target_link_directories(xattrplaying_plugin PRIVATE ${VLC_LIBRARY_DIRS})

    # Link against VLC libraries
    target_link_libraries(xattrplaying_plugin
            PRIVATE
            ${VLC_LIBVLC_LIBRARY}
            ${VLC_VLCCORE_LIBRARY})

    # Set the output directory for the shared library
    set_target_properties(xattrplaying_plugin PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

    # Set installation directory for the shared library
    install(TARGETS xattrplaying_plugin LIBRARY DESTINATION "${VLC_PLUGIN_INSTALL_DIR}")
else()
    message(WARNING "VLC not found, disabling plugin build")
endif()

if(BUILD_TESTING)
    add_executable(tag_utils_tests
            tests/tag_utils_tests.c
            tag_utils.c
            tag_utils.h)
    target_link_libraries(tag_utils_tests PRIVATE m)
    add_test(NAME tag_utils_tests COMMAND tag_utils_tests)
endif()
