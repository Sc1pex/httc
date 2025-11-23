const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const httc_lib = b.addLibrary(.{
        .name = "httc",
        .root_module = b.createModule(
            .{
                .target = target,
                .optimize = optimize,
                .link_libcpp = true,
            },
        ),
    });

    httc_lib.installHeadersDirectory(b.path("include"), "httc", .{
        .include_extensions = &.{".h"},
    });
    httc_lib.addCSourceFiles(.{
        .files = &.{
            "src/headers.cpp",
            "src/percent_encoding.cpp",
            "src/request.cpp",
            "src/request_parser.cpp",
            "src/response.cpp",
            "src/router.cpp",
            "src/server.cpp",
            "src/status.cpp",
            "src/uri.cpp",
        },
        .flags = &.{
            "-std=c++23",
            "-Iinclude",

            // https://github.com/ziglang/zig/issues/25455
            "-U_LIBCPP_ENABLE_CXX17_REMOVED_UNEXPECTED_FUNCTIONS",

            // Used to generate compile_commands.json fragments
            "-gen-cdb-fragment-path",
            "cdb-frags",
        },
    });

    const asio_dep = b.dependency("asio", .{});
    const asio_lib = asio_dep.artifact("asio");

    httc_lib.linkLibrary(asio_lib);

    b.installArtifact(httc_lib);

    const cdb_step = b.step("cdb", "Compile CDB fragments into compile_commands.json");
    cdb_step.makeFn = collect_cdb_fragments;
    cdb_step.dependOn(&httc_lib.step);
    b.getInstallStep().dependOn(cdb_step);
}

// Taken from https://zacoons.com/blog/2025-02-16-how-to-get-clang-lsp-working-with-zig/
fn collect_cdb_fragments(s: *std.Build.Step, _: std.Build.Step.MakeOptions) !void {
    // Open the compile_command.json file
    const outf = try std.fs.cwd().createFile("compile_commands.json", .{});
    defer outf.close();

    // Open the cdb-frags dir
    var dir = std.fs.cwd().openDir("cdb-frags", .{ .iterate = true }) catch {
        std.debug.print("Failed to open ./cdb-frags/", .{});
        return;
    };
    defer dir.close();

    // Iterate over the CDB fragments and add them to compile_commands.json
    try outf.writeAll("[");
    var iter = dir.iterate();

    const alloc = s.owner.allocator;

    while (try iter.next()) |entry| {
        const fpath = try std.fmt.allocPrint(std.heap.page_allocator, "cdb-frags/{s}", .{entry.name});
        const f = try std.fs.cwd().openFile(fpath, .{});

        const contents = try f.readToEndAlloc(alloc, 1024 * 1024);
        try outf.seekFromEnd(0);
        try outf.writeAll(contents);
    }
    try outf.writeAll("]");
}
