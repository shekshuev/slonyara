const std = @import("std");
const native_sdk = @import("native_sdk");

pub fn build(b: *std.Build) void {
    const app = native_sdk.addAppArtifacts(b, b.dependency("native_sdk", .{}), .{ .name = "slonyara" });
    if (app.exe.root_module.resolved_target.?.result.os.tag == .macos) {
        // Match the SDK host's Apple headers and Zig 0.16 ObjC sanitizer workaround.
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
