#!/bin/bash
set -e

echo "Building FlowZone CI..."

# 1. Build Web Client
echo "Building Web Client..."
cd src/web_client
npm install
npm run build
npm run test
cd ../..

# 2. Check C++ Libs
if [ ! -f "libs/civetweb/v1.16.tar.gz" ]; then
    echo "Error: CivetWeb missing"
    exit 1
fi

# 3. Build C++ (if Xcode project exists)
XCODE_PROJ="Builds/MacOSX/FlowZone.xcodeproj"
if [ -d "$XCODE_PROJ" ]; then
    echo "Xcode project found. Building Standalone..."
    xcodebuild -project "$XCODE_PROJ" -scheme "FlowZone - Standalone Plugin" -configuration Debug build
else
    echo "⚠️  Xcode project not found at $XCODE_PROJ"
    echo "👉  ACTION REQUIRED: Open FlowZone.jucer in Projucer and 'Save Project to Open in IDE' to generate it."
fi

echo "✅  CI Prep Verification Complete."
