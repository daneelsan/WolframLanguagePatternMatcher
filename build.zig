const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});

    const optimize = b.standardOptimizeOption(.{});

    // TODO: Try to find this path automatically
    // -Dwolfram-app=/Users/daniels/wr/stash/kern/Build/Layout/Mathematica.app;
    const default_wolfram_app_path: std.Build.LazyPath = .{ .cwd_relative = "/Applications/Wolfram.app/" };
    const wolfram_app_path = b.option(
        std.Build.LazyPath,
        "wolfram-app",
        "Path to the Wolfram Mathematica application.",
    ) orelse default_wolfram_app_path;

    const wolfram_system_files_path = wolfram_app_path.path(b, "Contents/SystemFiles/");

    // This creates a "module", which represents a collection of source files alongside
    // some compilation options, such as optimization mode and linked system libraries.
    // Every executable or library we compile will be based on one or more modules.
    const liblink_lib_mod = b.createModule(.{
        .root_source_file = b.path("PatternVirtualMachine/src/ObjectFactory.zig"),
        .target = target,
        .optimize = optimize,
    });

    // Add LibraryLink.c to the module, which is the LibraryLink entry file.
    liblink_lib_mod.addCSourceFile(.{ .file = b.path("PatternVirtualMachine/src/LibraryLink.c") });

    // Now, we will create a static library based on the module we created above.
    // This creates a `std.Build.Step.Compile`, which is the build step responsible
    // for actually invoking the compiler.
    const liblink_lib = b.addLibrary(.{
        .linkage = .dynamic,
        .name = "PatternMatcherVirtualMachine",
        .root_module = liblink_lib_mod,
    });

    // Link against the C standard library.
    liblink_lib.linkLibC();

    // Add the include paths for the Wolfram Engine headers.
    const wolfram_c_headers = wolfram_system_files_path.path(b, "IncludeFiles/C/");
    liblink_lib.addIncludePath(wolfram_c_headers);
    // Link against the Wolfram RTL library.
    // liblink_lib.linkSystemLibrary("WolframRTL");

    // Add library paths for Wolfram libraries (mainly the WolframEngine library).
    const wolfram_libraries = wolfram_system_files_path.path(b, "Libraries/MacOSX-ARM64/");
    liblink_lib.addLibraryPath(wolfram_libraries);
    liblink_lib.linkSystemLibrary("WolframEngine");

    const wolfram_dev_kit_path = wolfram_system_files_path.path(b, "Links/WSTP/DeveloperKit/MacOSX-ARM64/CompilerAdditions/");
    liblink_lib.addIncludePath(wolfram_dev_kit_path);
    liblink_lib.addFrameworkPath(wolfram_dev_kit_path);
    liblink_lib.linkFramework("wstp");

    // This declares intent for the library to be installed into the standard
    // location when the user invokes the "install" step (the default step when
    // running `zig build`).
    b.installArtifact(liblink_lib);
}
