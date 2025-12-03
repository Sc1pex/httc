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

    httc_lib.addIncludePath(b.path("include"));
    httc_lib.installHeadersDirectory(b.path("include"), "httc", .{
        .include_extensions = &.{".h"},
    });
    httc_lib.addCSourceFiles(.{
        .root = b.path("src"),
        .files = &.{
            "headers.cpp",
            "percent_encoding.cpp",
            "request.cpp",
            "request_parser.cpp",
            "response.cpp",
            "router.cpp",
            "server.cpp",
            "status.cpp",
            "uri.cpp",
            "utils/file_server.cpp",
            "utils/mime.cpp",
        },
        .flags = CXX_FLAGS,
    });

    const asio_dep = b.dependency("asio", .{});
    const asio_lib = asio_dep.artifact("asio");

    httc_lib.linkLibrary(asio_lib);

    // Make asio headers available to consumers of httc
    httc_lib.installed_headers.appendSlice(asio_lib.installed_headers.items) catch unreachable;

    b.installArtifact(httc_lib);

    const cdb_step = b.step("cdb", "Compile CDB fragments into compile_commands.json");
    cdb_step.makeFn = collect_cdb_fragments;
    cdb_step.dependOn(&httc_lib.step);
    b.getInstallStep().dependOn(cdb_step);

    const catch2_dep = b.dependency("catch2", .{
        .target = target,
        .optimize = optimize,
    });
    const catch2_lib = catch2_dep.artifact("Catch2");
    const catch2_main = catch2_dep.artifact("Catch2WithMain");

    const test_step = b.step("test", "Run tests");
    const test_exe = b.addExecutable(.{
        .name = "test",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });
    const run_test = b.addRunArtifact(test_exe);

    test_exe.addCSourceFiles(.{
        .root = b.path("test"),
        .files = &.{
            "headers.cpp",
            "percent_encoding.cpp",
            "request_parser.cpp",
            "router.cpp",
            "uri.cpp",
        },
        .flags = CXX_FLAGS,
    });
    test_exe.linkLibrary(asio_lib);
    test_exe.linkLibrary(httc_lib);
    test_exe.linkLibrary(catch2_lib);
    test_exe.linkLibrary(catch2_main);
    test_step.dependOn(&run_test.step);
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

const CXX_FLAGS = &.{
    "-std=c++23",
    "-Wall",
    "-Wextra",

    // https://github.com/ziglang/zig/issues/25455
    "-U_LIBCPP_ENABLE_CXX17_REMOVED_UNEXPECTED_FUNCTIONS",

    // Used to generate compile_commands.json fragments
    "-gen-cdb-fragment-path",
    "cdb-frags",
};
