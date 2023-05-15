# Sets the minimum version of CMake required to build the native library.
cmake_minimum_required(VERSION 3.4.1)

#Specify the so generation directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

include_directories(
        ./
)

aux_source_directory(./ SRC_JNI)

add_library( # Sets the name of the library.
        ipc-tools
        # Sets the library as a shared library.
        SHARED
        # Provides a relative path to your source file(s).
        ${SRC_JNI}
        )
