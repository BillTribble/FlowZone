#!/bin/bash
set -e
echo "Building FlowZone VST3..."
xcodebuild -project Builds/MacOSX/FlowZone.xcodeproj -scheme "FlowZone - VST3" -configuration Release build
