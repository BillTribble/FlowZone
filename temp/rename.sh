#!/bin/bash
set -e

echo "1. Marking bead as in progress..."
br update bd-262 --status=in_progress

echo "2. Copying Spec..."
cp archive/v1/Spec/Spec_FlowZone_Looper1.6.md Spec/Spec_Samsara_Looper2.1.md
rm -f Spec/Spec_FlowZone_V2_Prototype.md

echo "3. Renaming Projucer..."
mv FlowZone.jucer Samsara.jucer || true

echo "4. Deleting old build..."
rm -rf build/
rm -rf temp_build/

echo "5. Global Find and Replace (FlowZone -> Samsara) in tracked files..."
# Find all files in src, Spec, and root txt/cmake files
find src Spec -type f -name "*.*" -exec sed -i '' 's/FlowZone/Samsara/g' {} +
find src Spec -type f -name "*.*" -exec sed -i '' 's/flowzone/samsara/g' {} +
sed -i '' 's/FlowZone/Samsara/g' CMakeLists.txt
sed -i '' 's/flowzone/samsara/g' CMakeLists.txt
sed -i '' 's/FlowZone/Samsara/g' Samsara.jucer || true
sed -i '' 's/flowzone/samsara/g' Samsara.jucer || true

echo "6. Regenerating CMake under new name..."
cmake -B build -G Xcode

echo "Done!"
