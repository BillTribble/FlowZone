#!/bin/bash
set -e

echo "Building FlowZone CI..."

# 1. Build Web Client
echo "Building Web Client..."
cd src/web_client
npm install
npm run build
npm test
cd ../..

# 2. Build Engine (Standalone)
echo "Building Standalone..."
if [[ "$OSTYPE" == "darwin"* ]]; then
    # MacOS
    # Assume Projucer has generated the Xcode project
    # If not, we might need to run Projucer --resave FlowZone.jucer
    
    # Verify Xcode project exists
    if [ -d "Builds/MacOSX/FlowZone.xcodeproj" ]; then
        xcodebuild -project Builds/MacOSX/FlowZone.xcodeproj -scheme "FlowZone - Standalone Plugin" -configuration Release build
    else
        echo "Xcode project not found. Please open FlowZone.jucer and save to generate it."
        # Optional: try to run Projucer if available
        # /Applications/JUCE/Projucer.app/Contents/MacOS/Projucer --resave FlowZone.jucer
    fi
else
    echo "Skipping Engine build (non-macOS currently)"
fi

echo "CI Build Complete."
