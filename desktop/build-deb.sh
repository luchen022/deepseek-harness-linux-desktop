#!/usr/bin/env bash
set -euo pipefail
root=$(realpath "${2:?Provide upstream source directory}")
wrapper=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
version=${1:?Provide a Debian version such as 1.0.0}
[[ "$version" =~ ^[0-9]+(\.[0-9]+){2}$ ]] || { echo 'Invalid version' >&2; exit 1; }
[ "$(dpkg --print-architecture)" = amd64 ] || { echo 'Only amd64 is supported' >&2; exit 1; }
cd "$root"
test -f apps/cli/lib/bin.js
test -f apps/web/dist/index.html
out="$root/.desktop-build-linux"
rm -rf "$out"
mkdir -p "$out/pkg/DEBIAN" "$out/pkg/opt/deepseek-harness/bin" "$out/pkg/opt/deepseek-harness/node/bin" "$out/pkg/usr/bin" "$out/pkg/usr/share/applications"
gcc "$wrapper/main.c" -O2 -Wall -Wextra -o "$out/pkg/opt/deepseek-harness/bin/deepseek-harness" $(pkg-config --cflags --libs webkit2gtk-4.1)
cp "$(command -v node)" "$out/pkg/opt/deepseek-harness/node/bin/node"
rsync -a --exclude='/.git' --exclude='/.desktop-build-linux' --exclude='/node_modules/.cache' --exclude='*.map' "$root/" "$out/pkg/opt/deepseek-harness/dsh/"
ln -s /opt/deepseek-harness/bin/deepseek-harness "$out/pkg/usr/bin/deepseek-harness"
cat > "$out/pkg/DEBIAN/control" <<CONTROL
Package: deepseek-harness-linux
Version: $version
Architecture: amd64
Maintainer: Community desktop packaging
Depends: libwebkit2gtk-4.1-0, libgtk-3-0, libjavascriptcoregtk-4.1-0, libstdc++6, libc6, libgcc-s1
Section: devel
Priority: optional
Description: Community Linux desktop wrapper for DeepSeek Harness
 A WebKitGTK window and bundled dsh web runtime.
CONTROL
cat > "$out/pkg/usr/share/applications/deepseek-harness.desktop" <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=DeepSeek Harness
Comment=Community Linux desktop wrapper for DeepSeek Harness
Exec=/opt/deepseek-harness/bin/deepseek-harness
Terminal=false
Categories=Development;Utility;
DESKTOP
mkdir -p "$wrapper/../dist-linux"
dpkg-deb --root-owner-group -Zzstd -b "$out/pkg" "$wrapper/../dist-linux/deepseek-harness_${version}_amd64.deb"
