const std = @import("std");

const native_sdk = @import("native_sdk");
const runner = @import("runner");

const controller = @import("app/controller.zig");
const windows = @import("ui/windows/windows.zig");

pub const Model = controller.Model;
pub const Msg = controller.Msg;

pub const panic = std.debug.FullPanic(native_sdk.debug.capturePanic);

pub fn main(init: std.process.Init) !void {
    const app = try controller.create(std.heap.page_allocator, init.io);
    defer app.destroy();

    try runner.runWithOptions(controller.application(app), .{
        .app_name = "slonyara",
        .window_title = "Slonyara",
        .bundle_id = "dev.shekshuev.slonyara",
        .icon_path = "assets/icon.png",
        .default_frame = native_sdk.geometry.RectF.init(0, 0, windows.intro.width, windows.intro.height),
        .js_window_api = false,
        .security = .{
            .permissions = &.{ native_sdk.security.permission_command, native_sdk.security.permission_view },
            .navigation = .{ .allowed_origins = &.{ "zero://inline", "zero://app" } },
        },
    }, init);
}

test {
    _ = windows;
}
