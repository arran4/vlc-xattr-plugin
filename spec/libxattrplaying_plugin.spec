Name: libxattrplaying_plugin
Version: 0.2.0
Release: 1%{?dist}
Summary: A plugin for VLC which adds the xattr tag 'seen' to the `user.xdg.tags` list anytime you watch a video

License: MIT
URL: https://github.com/arran4/vlc-xattr-plugin

%description
This is a plugin for VLC which adds the xattr tag 'seen' to the `user.xdg.tags` list anytime you watch a video.

%prep
# No need for build stage as we build it earlier and we are non compliant

%build
# No need for build stage as we build it earlier and we are non compliant

%install
mkdir -p %{buildroot}/usr/lib64/vlc/plugins/misc/
cp libxattrplaying_plugin.so %{buildroot}/usr/lib64/vlc/plugins/misc/

%files
/usr/lib64/vlc/plugins/misc/libxattrplaying_plugin.so

%changelog
* Sat Jan 01 2022 Arran Ubels <arran4@gmail.com> - 1.0-1
- Initial release - Stub change log