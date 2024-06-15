# vlc xattr plugin

This is a plugin for VLC which adds the xattr tag 'seen' to the `user.xdg.tags` list anytime you watch a video.

![img.png](doc/img.png)

Currently for any system that supports `setxattr` and `getxattr` which AFAIK is just Linux, but Mac might, and Windows might via WSL. (It should be possible to port it.)

# How to build it? 

Currently I'm using: (To build and install)
```
cmake --build ./cmake-build-debug --target all -j 14 && sudo cp -vf "./lib/libxattrplaying_plugin.so" "/usr/lib64/vlc/plugins/misc/libxattrplaying_plugin.so"
```

Requirements:
* VLC headers
* Standard C Development Libraries for Linux
* Cmake

# How to use it

You will need to enable it in the settings once you have placed it in the correct directory:

![Screenshot_20240615_221813.png](doc/Screenshot_20240615_221813.png)

