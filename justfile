clean:
    rm -rf .cache build

setup:
    CC=clang CXX=clang++ cmake --preset=vcpkg -S . -B build

build:
    cmake --build build

build_release:
    cmake --build build --config Release

# Build with tests and examples
build_dev:
    CC=clang CXX=clang++ cmake --preset=vcpkg -S . -B build -DHTTC_BUILD_TESTS=ON -DHTTC_BUILD_EXAMPLES=ON
    cmake --build build

test:
    cmake --build build --target tests
    ./build/test/tests

dev: clean build_dev test
