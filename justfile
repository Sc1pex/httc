clean:
    rm -rf .cache build

setup:
    CC=clang CXX=clang++ cmake --preset=vcpkg -S . -B build

build:
    cmake --build build

build_release:
    cmake --build build --config Release
