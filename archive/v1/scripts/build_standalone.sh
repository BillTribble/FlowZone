#!/bin/bash
set -e
echo "Building FlowZone Standalone..."
xcodebuild -project Builds/MacOSX/FlowZone.xcodeproj -scheme "FlowZone - Standalone Plugin" -configuration Release build
