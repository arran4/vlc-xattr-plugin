Name: libxattrplaying_plugin
Version: 1.0.1
Release: 1%{?dist}
Summary: A plugin for VLC which adds the xattr tag 'seen' to the `user.xdg.tags` list anytime you watch a video

License: MIT
URL: https://github.com/arran4/vlc-xattr-plugin
BuildRequires: vlc-devel
BuildRequires: cmake
BuildRequires: gcc
BuildRequires: make

%description
This is a plugin for VLC which adds the xattr tag 'seen' to the `user.xdg.tags` list anytime you watch a video.

%prep
%setup -q

%build
%cmake -B build -S . -DVLC_PLUGIN_INSTALL_DIR=%{_libdir}/vlc/plugins/misc
%cmake_build -B build

%install
%cmake_install -B build

%files
%{_libdir}/vlc/plugins/misc/libxattrplaying_plugin.so

%changelog
* Sat Jan 01 2022 Arran Ubels <arran4@gmail.com> - 1.0-1
- Initial release - Stub change log
