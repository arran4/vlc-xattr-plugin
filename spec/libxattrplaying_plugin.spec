Name: libxattrplaying_plugin
Version: 1.0.1
Release: 1%{?dist}
Summary: A plugin for VLC which adds the xattr tag 'seen' to the `user.xdg.tags` list anytime you watch a video

License: MIT
URL: https://github.com/arran4/vlc-xattr-plugin
Source0: %{name}-%{version}.tar.gz
BuildRequires: vlc-devel
BuildRequires: cmake
BuildRequires: gcc
BuildRequires: make

%description
This is a plugin for VLC which adds the xattr tag 'seen' to the `user.xdg.tags` list anytime you watch a video.

%prep
%setup -q

%build
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DVLC_PLUGIN_INSTALL_DIR=%{_libdir}/vlc/plugins/misc
cmake --build build

%install
mkdir -p %{buildroot}%{_defaultlicensedir}/%{name}
DESTDIR="%{buildroot}" cmake --install build
cp LICENSE %{buildroot}%{_defaultlicensedir}/%{name}/

%files
%license %{_defaultlicensedir}/%{name}/LICENSE
%{_libdir}/vlc/plugins/misc/libxattrplaying_plugin.so

%changelog
* Sat Jan 01 2022 Arran Ubels <arran4@gmail.com> - 1.0-1
- Initial release - Stub change log
