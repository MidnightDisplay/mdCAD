#!/bin/bash
set -e

# Android SDK/NDK setup for macOS (no Android Studio required)
# This script installs the minimal components needed to build Android apps

ANDROID_HOME="${ANDROID_HOME:-$HOME/Android/sdk}"
CMDLINE_TOOLS_URL="https://dl.google.com/android/repository/commandlinetools-mac-11076708_latest.zip"
NDK_VERSION="26.1.10909125"
BUILD_TOOLS_VERSION="34.0.0"
PLATFORM_VERSION="34"
CMAKE_VERSION="3.22.1"

echo "=== Android SDK Setup ==="
echo "ANDROID_HOME: $ANDROID_HOME"
echo ""

# 1. Check for Java 17
echo "Checking for Java 17..."
if command -v java &> /dev/null; then
    JAVA_VER=$(java -version 2>&1 | head -n 1 | awk -F '"' '{print $2}' | cut -d'.' -f1)
    if [ "$JAVA_VER" = "17" ] || [ "$JAVA_VER" = "21" ]; then
        echo "  Java $JAVA_VER found, OK"
    else
        echo "  Java $JAVA_VER found, but Java 17+ recommended"
        echo "  Installing Java 17 via Homebrew..."
        brew install openjdk@17
        echo 'export PATH="/opt/homebrew/opt/openjdk@17/bin:$PATH"' >> ~/.zshrc
        export PATH="/opt/homebrew/opt/openjdk@17/bin:$PATH"
    fi
else
    echo "  Java not found, installing Java 17 via Homebrew..."
    brew install openjdk@17
    echo 'export PATH="/opt/homebrew/opt/openjdk@17/bin:$PATH"' >> ~/.zshrc
    export PATH="/opt/homebrew/opt/openjdk@17/bin:$PATH"
fi

# 2. Download command-line tools
echo ""
echo "Checking for Android command-line tools..."
mkdir -p "$ANDROID_HOME/cmdline-tools"
if [ ! -d "$ANDROID_HOME/cmdline-tools/latest" ]; then
    echo "  Downloading Android command-line tools..."
    curl -L -o /tmp/cmdline-tools.zip "$CMDLINE_TOOLS_URL"
    echo "  Extracting..."
    unzip -q /tmp/cmdline-tools.zip -d /tmp/
    mv /tmp/cmdline-tools "$ANDROID_HOME/cmdline-tools/latest"
    rm /tmp/cmdline-tools.zip
    echo "  Installed to $ANDROID_HOME/cmdline-tools/latest"
else
    echo "  Already installed at $ANDROID_HOME/cmdline-tools/latest"
fi

# Set PATH for sdkmanager
export PATH="$ANDROID_HOME/cmdline-tools/latest/bin:$PATH"

# 3. Accept licenses
echo ""
echo "Accepting Android SDK licenses..."
yes | sdkmanager --licenses > /dev/null 2>&1 || true

# 4. Install components
echo ""
echo "Installing SDK components..."
echo "  - platform-tools (adb)"
echo "  - build-tools;$BUILD_TOOLS_VERSION"
echo "  - platforms;android-$PLATFORM_VERSION"
echo "  - ndk;$NDK_VERSION"
echo "  - cmake;$CMAKE_VERSION"
echo ""

sdkmanager \
    "platform-tools" \
    "build-tools;$BUILD_TOOLS_VERSION" \
    "platforms;android-$PLATFORM_VERSION" \
    "ndk;$NDK_VERSION" \
    "cmake;$CMAKE_VERSION"

# 5. Shell profile setup
PROFILE_FILE="$HOME/.zshrc"
echo ""
echo "Updating shell profile ($PROFILE_FILE)..."
if ! grep -q "ANDROID_HOME" "$PROFILE_FILE" 2>/dev/null; then
    echo "" >> "$PROFILE_FILE"
    echo "# Android SDK (added by setup-android.sh)" >> "$PROFILE_FILE"
    echo "export ANDROID_HOME=\"$ANDROID_HOME\"" >> "$PROFILE_FILE"
    echo "export ANDROID_NDK_HOME=\"\$ANDROID_HOME/ndk/$NDK_VERSION\"" >> "$PROFILE_FILE"
    echo 'export PATH="$ANDROID_HOME/cmdline-tools/latest/bin:$PATH"' >> "$PROFILE_FILE"
    echo 'export PATH="$ANDROID_HOME/platform-tools:$PATH"' >> "$PROFILE_FILE"
    echo "  Added Android environment variables"
else
    echo "  ANDROID_HOME already set in $PROFILE_FILE, skipping"
fi

# 6. Verify installation
echo ""
echo "=== Verification ==="
echo ""
echo "SDK Manager version:"
sdkmanager --version
echo ""
echo "Installed packages:"
sdkmanager --list_installed 2>/dev/null | head -20
echo ""
echo "NDK location: $ANDROID_HOME/ndk/$NDK_VERSION"
if [ -d "$ANDROID_HOME/ndk/$NDK_VERSION" ]; then
    echo "  NDK directory exists, OK"
else
    echo "  WARNING: NDK directory not found!"
fi

echo ""
echo "=== Setup Complete ==="
echo ""
echo "Next steps:"
echo "  1. Run: source ~/.zshrc"
echo "  2. Connect an Android device with USB debugging enabled"
echo "  3. Run: adb devices"
echo ""
echo "Build Android APK with:"
echo "  cd android && ./gradlew assembleDebug"
echo ""
