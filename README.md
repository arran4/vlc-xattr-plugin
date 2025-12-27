# vlc xattr plugin

This is a plugin for VLC which adds the xattr tag 'seen' to the `user.xdg.tags` list anytime you watch a video.

```bash
% attr -g xdg.tags ~/Downloads/14⧸6⧸2024\ \[NU2402V166S00\].mp4
Attribute "xdg.tags" had a 4 byte value for /home/arran/Downloads/14⧸6⧸2024 [NU2402V166S00].mp4:
seen
```

![img.png](doc/img.png)

Currently for any system that supports `setxattr` and `getxattr` which AFAIK is just Linux, but Mac might, and Windows might via WSL. (It should be possible to port it.)

# How to build it? 

Requirements:
* VLC headers
* Standard C Development Libraries for Linux
* CMake

## Configure (out of tree)

Use an out-of-tree build directory so generated files do not clutter the source tree:

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

You can override the plugin install location during configuration with:

```
cmake -S . -B build -DVLC_PLUGIN_INSTALL_DIR=/your/custom/path
```

## Build

Compile the plugin using the configured build directory:

```
cmake --build build
```

## Install

Install the compiled plugin to the chosen prefix (defaults to `/usr/local`):

```
sudo cmake --install build
```

For packaging or staging installs, use `DESTDIR` to stage into a temporary root:

```
DESTDIR=/tmp/vlc-xattr-staging cmake --install build
sudo DESTDIR=/tmp/pkg-root cmake --install build
```

The install step places `libxattrplaying_plugin.so` into `${VLC_PLUGIN_INSTALL_DIR}` (which defaults to `${CMAKE_INSTALL_LIBDIR}/vlc/plugins/misc`).

### Common package install paths

Distribution packaging may set different `libdir` defaults. Typical plugin destinations are:

* Debian / Ubuntu: `/usr/lib/x86_64-linux-gnu/vlc/plugins/misc`
* Fedora / RHEL / CentOS: `/usr/lib64/vlc/plugins/misc`
* Arch Linux: `/usr/lib/vlc/plugins/misc`

You can verify the active plugin path with `pkg-config --variable=pluginsdir vlc` or override it via `-DVLC_PLUGIN_INSTALL_DIR` when configuring CMake.

# How to use it

You will need to enable it in the settings once you have placed it in the correct directory:

![Screenshot_20240615_221813.png](doc/Screenshot_20240615_221813.png)
