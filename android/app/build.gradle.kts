plugins {
    id("com.android.application")
}

android {
    namespace = "com.midnightdisplay.skltmp"
    compileSdk = 34

    ndkVersion = "27.0.12077973"

    defaultConfig {
        applicationId = "com.midnightdisplay.skltmp"
        minSdk = 30
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"

        ndk {
            abiFilters += listOf("arm64-v8a", "armeabi-v7a", "x86_64")
        }

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++17"
                arguments += listOf(
                    "-DANDROID_STL=c++_shared",
                    "-DCMAKE_BUILD_TYPE=Release",
                    "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON"
                )
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
        debug {
            isDebuggable = true
            isJniDebuggable = true
        }
    }

    // 16KB page alignment for Android 15+ compatibility
    packaging {
        jniLibs {
            useLegacyPackaging = false
        }
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}
