clean:
    rm -rf .cache build

setup:
    CC=clang CXX=clang++ cmake --preset=vcpkg -S . -B build

setup_dev:
    CC=clang CXX=clang++ cmake --preset=vcpkg-dev -S . -B build

build:
    cmake --build build

build_release:
    cmake --build build --config Release

test:
    cmake --build build --target tests
    ./build/test/tests

dev: clean setup_dev build test
