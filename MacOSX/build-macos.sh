#!/bin/bash

./setup-vcpkg.sh

xcodebuild -project QuakeSpasm.xcodeproj -target QuakeSpasm-Spiked-SDL2

cat <<EOF > build/Release/Quakespasm-Spiked-Revision.txt
Git URL:      $(git config --get remote.origin.url)
Git Revision: $(git rev-parse HEAD)
Git Date:     $(git show --no-patch --no-notes --pretty='%ai' HEAD)
Compile Date: $(date)
EOF

# zip the files in `build/Release` to create the final archive for distribution
cd build/Release
rm Quakespasm-Spiked-macos.zip
zip --symlinks --recurse-paths Quakespasm-Spiked-macos.zip *
