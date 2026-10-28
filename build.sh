#!/bin/zsh
# Builds FanBar.app next to this script.
set -e
cd "${0:A:h}"
APP=FanBar.app
rm -rf build "$APP"
mkdir -p build "$APP/Contents/MacOS"
clang -O2 -c sensors.c -o build/sensors.o
swiftc -O -import-objc-header sensors.h main.swift build/sensors.o \
  -framework IOKit -framework CoreFoundation -framework AppKit \
  -o "$APP/Contents/MacOS/FanBar"
cat > "$APP/Contents/Info.plist" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleName</key><string>FanBar</string>
  <key>CFBundleIdentifier</key><string>local.fanbar</string>
  <key>CFBundleExecutable</key><string>FanBar</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>CFBundleShortVersionString</key><string>1.0</string>
  <key>LSMinimumSystemVersion</key><string>13.0</string>
  <key>LSUIElement</key><true/>
</dict>
</plist>
EOF
codesign --force --sign - "$APP"
rm -rf build
echo "Built $PWD/$APP"
