libjpeg
=======

A complete implementation of 10918-1 (JPEG) coming from jpeg.org (the ISO group) with extensions for HDR standardized as 18477 (JPEG XT)

This release also includes the "JPEG on Steroids" improvements implemented for the ICIP 2016 Grand Challenge on Image Compression. For ideal visual performance, run jpeg as follows:

jpeg -q <quality> -oz -v -qt 3 -h -s 1x1,2x2,2x2 input.ppm output.jpg

General build steps when using CMake
------------------------------------

1. Clone the libjpeg git repository to your own system.

1. CMake can be used to build in-source and out-of-source. The recommendation is to build libjpeg out-of-source:

1. To build the “debug” or "release" version, use the following steps:
    - cd into the root directory of your cloned libjpeg directory
    - ```mkdir build```
    - ```cd build```
    - ```cmake -DCMAKE_BUILD_TYPE=Debug ..``` (static library debug) or
    - ```cmake -DCMAKE_BUILD_TYPE=Debug -DBUILD_SHARED_LIBS=On ..``` (shared library debug) or
    - ```cmake -DCMAKE_BUILD_TYPE=Release ..``` (static library release) or
    - ```cmake -DCMAKE_BUILD_TYPE=Relase -DBUILD_SHARED_LIBS=On ..``` (shared library release)
    - ```make``` (or ```ninja``` on Windows)

    **Note:** On Windows pass -G Ninja to the cmake command line to force
the creation of ninja build files. The default on the Windows platform is to generate Visual Studio project files. Newer versions of Visual Studio can also open directly the CMakeLists.txt build script.
