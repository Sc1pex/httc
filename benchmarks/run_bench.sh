#!/bin/sh

cd benchmarks/rust_compare
cargo build --release
cd ../../
zig build -Doptimize=ReleaseFast

clear

hyperfine --warmup 3 './zig-out/bin/parser_bench sm 1000000' './benchmarks/rust_compare/target/release/rust_compare sm 1000000'
hyperfine --warmup 3 './zig-out/bin/parser_bench lg 200000' './benchmarks/rust_compare/target/release/rust_compare lg 200000'
hyperfine --warmup 3 './zig-out/bin/parser_bench xl 10000' './benchmarks/rust_compare/target/release/rust_compare xl 10000'
