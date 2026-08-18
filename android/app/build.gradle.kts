plugins {
    id("com.android.application")
}

android {
    namespace = "net.rlgame"
    compileSdk = 34

    defaultConfig {
        applicationId = "net.rlgame"
        minSdk = 24
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"

        ndk {
            abiFilters.addAll(listOf("arm64-v8a", "armeabi-v7a", "x86_64", "x86"))
        }

        externalNativeBuild {
            cmake {
                cppFlags += listOf("-std=c++17", "-DR_CSTL_HEAP_DEBUG=1")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
        debug {
            isDebuggable = true
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}

dependencies {
    implementation("androidx.appcompat:appcompat:1.6.1")
}

tasks.register<Copy>("copyNativeLibs") {
    description = "Copy rlgame_lib static library to jniLibs folder"
    
    val buildDir = file("../../build/Debug")
    val jniLibsDir = file("src/main/jniLibs")
    
    val abis = mapOf(
        "arm64-v8a" to "aarch64-linux-android",
        "armeabi-v7a" to "armv7-linux-androideabi",
        "x86_64" to "x86_64-linux-android",
        "x86" to "i686-linux-android"
    )
    
    abis.forEach { (abi, _) ->
        val libFile = file("$buildDir/librlgame_lib.a")
        if (libFile.exists()) {
            from(libFile)
            into(file("$jniLibsDir/$abi"))
            rename { "librlgame_lib.a" }
        }
    }
}

tasks.named("preBuild") {
    dependsOn("copyNativeLibs")
}
