const std = @import("std");

const native_sdk = @import("native_sdk");

pub fn build(b: *std.Build) void {
    const app = native_sdk.addAppArtifacts(
        b,
        b.dependency("native_sdk", .{}),
        .{ .name = "slonyara" },
    );

    const target = app.exe.root_module.resolved_target.?;
    const optimize = app.exe.root_module.optimize.?;

    const pg_module = b.dependency("pg", .{}).module("pg");

    const core_mod = b.createModule(.{
        .root_source_file = b.path("src/core/core.zig"),
        .target = target,
        .optimize = optimize,
        .imports = &.{
            .{ .name = "pg", .module = pg_module },
        },
    });

    app.exe.root_module.addImport("core", core_mod);
    app.tests.root_module.addImport("core", core_mod);

    for ([_]*std.Build.Module{ app.exe.root_module, app.tests.root_module }) |module| {
        module.addAnonymousImport("app_icon", .{ .root_source_file = b.path("assets/icon.png") });
        const runner = module.import_table.get("runner").?;
        module.addImport("app_manifest_zon", runner.import_table.get("app_manifest_zon").?);
    }
    if (app.exe.root_module.resolved_target.?.result.os.tag == .macos) {
        const flags: []const []const u8 = if (b.sysroot) |sysroot|
            &.{ "-fobjc-arc", "-fno-sanitize=builtin", "-ObjC", "-isysroot", sysroot, b.fmt("-I{s}/usr/include", .{sysroot}) }
        else
            &.{ "-fobjc-arc", "-fno-sanitize=builtin", "-ObjC" };
        for ([_]*std.Build.Module{ app.exe.root_module, app.tests.root_module }) |module| {
            module.link_libc = true;
            if (b.sysroot) |sysroot| {
                module.addFrameworkPath(.{ .cwd_relative = b.pathJoin(&.{ sysroot, "System/Library/Frameworks" }) });
            }
            module.addCSourceFile(.{
                .file = b.path("src/platform/macos/window_effects.m"),
                .flags = flags,
            });
            module.linkFramework("AppKit", .{});
            module.linkFramework("QuartzCore", .{});
        }
    }
}
