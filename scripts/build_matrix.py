#!/usr/bin/env python3
import argparse
import itertools
import os
import shutil
import subprocess
import sys
from typing import List, Tuple

sys.stdout.reconfigure(line_buffering=True)

# All available options
ALL_COMPILERS = ["clang", "gcc"]
ALL_BUILD_TYPES = ["debug", "release"]
ALL_SANITIZERS = ["none", "asan", "ubsan", "asan+ubsan"]

# Map user-friendly names to compiler sanitizer flags
SANITIZER_MAP = {
    "none": "none",
    "asan": "address",
    "ubsan": "undefined",
    "asan+ubsan": "address,undefined"
}

def run_cmd(cmd: List[str], env=None) -> bool:
    """Runs a shell command and returns True if successful, False otherwise."""
    try:
        result = subprocess.run(cmd, env=env, check=True)
        return result.returncode == 0
    except subprocess.CalledProcessError:
        return False

def build_and_test(compiler: str, build_type: str, sanitizer: str, run_tests: bool, clean: bool, no_examples: bool, unity: bool) -> Tuple[bool, str]:
    """Configures, builds, and tests a single matrix combination."""
    cc = "clang" if compiler == "clang" else "gcc"
    cxx = "clang++" if compiler == "clang" else "g++"

    # Determine unique build directory
    build_dir = f"build/{compiler}-{build_type}"

    # Targeted clean: only delete this config's directory
    if clean and os.path.exists(build_dir):
        print(f"🧹 Cleaning build directory: {build_dir}")
        shutil.rmtree(build_dir)

    # Set compiler env vars
    env = os.environ.copy()
    env["CXX"] = cxx
    env["CC"] = cc

    # CMake configure options (CMake build type needs to be capitalized like Debug or Release)
    cmake_flags = [
        "-G", "Ninja",
        "-B", build_dir,
        "-S", ".",
        f"-DCMAKE_BUILD_TYPE={build_type.capitalize()}",
        f"-DHTTC_BUILD_EXAMPLES={'OFF' if no_examples else 'ON'}",
        f"-DCMAKE_UNITY_BUILD={'ON' if unity else 'OFF'}",
    ]

    # Sanitizer flags (must explicitly clear flags when none is selected to overwrite cached values)
    sanitizer_flag = SANITIZER_MAP[sanitizer]
    if sanitizer_flag != "none":
        flag = f"-fsanitize={sanitizer_flag}"
        cmake_flags.append(f"-DCMAKE_CXX_FLAGS={flag}")
        cmake_flags.append(f"-DCMAKE_EXE_LINKER_FLAGS={flag}")
    else:
        cmake_flags.append("-DCMAKE_CXX_FLAGS=")
        cmake_flags.append("-DCMAKE_EXE_LINKER_FLAGS=")

    print(f"\n🚀 Configuring: Compiler={compiler} | Build={build_type} | Sanitizer={sanitizer}")
    if not run_cmd(["cmake"] + cmake_flags, env=env):
        return False, "Configure Failed"

    print(f"🔨 Building...")
    if not run_cmd(["cmake", "--build", build_dir]):
        return False, "Build Failed"

    if run_tests:
        print(f"🧪 Running Tests...")
        # Set runtime sanitizer flags
        if "asan" in sanitizer:
            env["ASAN_OPTIONS"] = "detect_leaks=1:color=always"
        if "ubsan" in sanitizer:
            env["UBSAN_OPTIONS"] = "color=always"

        if not run_cmd(["ctest", "--test-dir", build_dir, "--output-on-failure"], env=env):
            return False, "Tests Failed"

    return True, "Passed"

def main():
    parser = argparse.ArgumentParser(description="Multi-compiler build and test matrix runner.")
    parser.add_argument(
        "-c", "--compiler",
        choices=["all"] + ALL_COMPILERS, default="all",
        help="Compiler to run (default: all)"
    )
    parser.add_argument(
        "-b", "--build-type",
        choices=["all"] + ALL_BUILD_TYPES, default="all",
        help="Build type to run (default: all)"
    )
    parser.add_argument(
        "-s", "--sanitizer",
        choices=["all"] + ALL_SANITIZERS, default="all",
        help="Sanitizer to run (default: all)"
    )
    parser.add_argument("-t", "--test", action="store_true", help="Run tests for selected config(s)")
    parser.add_argument("-f", "--fail-fast", action="store_true", help="Stop execution at the first failure")
    parser.add_argument("--clean", action="store_true", help="Clean ONLY the build directory of running config(s)")
    parser.add_argument("--no-examples", action="store_true", help="Disable building examples to speed up compilation")
    parser.add_argument("--unity", action="store_true", help="Enable unity (jumbo) build to reduce compilation time")
    args = parser.parse_args()

    # Filter matrix options based on arguments
    compilers = ALL_COMPILERS if args.compiler == "all" else [args.compiler]
    build_types = ALL_BUILD_TYPES if args.build_type == "all" else [args.build_type]
    sanitizers = ALL_SANITIZERS if args.sanitizer == "all" else [args.sanitizer]

    # Generate all combinations to run
    matrix = list(itertools.product(compilers, build_types, sanitizers))
    results = []

    for compiler, build_type, sanitizer in matrix:
        success, reason = build_and_test(compiler, build_type, sanitizer, args.test, args.clean, args.no_examples, args.unity)
        results.append((compiler, build_type, sanitizer, success, reason))

        if not success and args.fail_fast:
            print("\n❌ Build failed and --fail-fast is enabled. Exiting.")
            break

    # Print final summary table
    print("\n" + "="*60)
    print(" BUILD SUMMARY")
    print("="*60)
    print(f"{'Compiler':<12} | {'Build Type':<10} | {'Sanitizer':<10} | {'Status'}")
    print("-"*60)
    for compiler, build_type, sanitizer, success, reason in results:
        status_str = f"✅ {reason}" if success else f"❌ {reason}"
        print(f"{compiler:<12} | {build_type:<10} | {sanitizer:<10} | {status_str}")
    print("="*60)

    # Exit with code 1 if any build failed
    if any(not r[3] for r in results):
        sys.exit(1)

if __name__ == "__main__":
    main()
