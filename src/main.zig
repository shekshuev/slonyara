const std = @import("std");
const builtin = @import("builtin");

const native_sdk = @import("native_sdk");
const canvas = native_sdk.canvas;
const geometry = native_sdk.geometry;
const runner = @import("runner");

const macos = @import("platform/macos/window_effects.zig");

const is_macos = builtin.os.tag == .macos;

pub const panic = std.debug.FullPanic(native_sdk.debug.capturePanic);

const canvas_label = "main-canvas";
const window_width: f32 = 480;
const window_height: f32 = 320;

const app_permissions = [_][]const u8{
    native_sdk.security.permission_command,
    native_sdk.security.permission_view,
};
const shell_views = [_]native_sdk.ShellView{
    .{
        .label = canvas_label,
        .kind = .gpu_surface,
        .fill = true,
        .role = "Slonyara canvas",
        .accessibility_label = "Slonyara",
        .gpu_backend = .metal,
        .gpu_pixel_format = .bgra8_unorm,
        .gpu_present_mode = .timer,
        .gpu_alpha_mode = if (is_macos) .premultiplied else .@"opaque",
        .gpu_color_space = .srgb,
        .gpu_vsync = true,
    },
};
const shell_windows = [_]native_sdk.ShellWindow{.{
    .label = "main",
    .title = "Slonyara",
    .width = window_width,
    .height = window_height,
    .transparent = is_macos,
    .views = &shell_views,
}};
const shell_scene: native_sdk.ShellConfig = .{ .windows = &shell_windows };

pub const Model = struct {
    appearance: native_sdk.platform.Appearance = .{},

    pub const view_unbound = .{"appearance"};
};

pub const Msg = union(enum) {
    noop,
    appearance_changed: native_sdk.platform.Appearance,

    pub const view_unbound = .{ "noop", "appearance_changed" };
};

fn update(model: *Model, msg: Msg) void {
    switch (msg) {
        .noop => {},
        .appearance_changed => |appearance| model.appearance = appearance,
    }
}

fn onAppearance(appearance: native_sdk.platform.Appearance) ?Msg {
    return .{ .appearance_changed = appearance };
}

fn tokens(model: *const Model) canvas.DesignTokens {
    var theme = canvas.DesignTokens.theme(.{
        .color_scheme = if (model.appearance.color_scheme == .dark) .dark else .light,
        .contrast = if (model.appearance.high_contrast) .high else .standard,
        .reduce_motion = model.appearance.reduce_motion,
    });
    theme.colors.background.a = 0;
    return theme;
}

fn onFrame(_: *const Model, _: native_sdk.platform.GpuFrame) ?Msg {
    macos.applyVibrancy(canvas_label);
    return null;
}

pub const AppUi = canvas.Ui(Msg);
pub const app_markup = @embedFile("app.native");

const SlonyaraApp = native_sdk.UiApp(Model, Msg);

pub fn main(init: std.process.Init) !void {
    const app_state = try SlonyaraApp.create(std.heap.page_allocator, .{
        .name = "slonyara",
        .scene = shell_scene,
        .canvas_label = canvas_label,
        .update = update,
        .tokens_fn = if (is_macos) tokens else null,
        .on_appearance = if (is_macos) onAppearance else null,
        .on_frame = if (is_macos) onFrame else null,
        .markup = .{
            .source = app_markup,
            .watch_path = "src/app.native",
            .io = init.io,
        },
    });
    defer app_state.destroy();

    try runner.runWithOptions(app_state.app(), .{
        .app_name = "slonyara",
        .window_title = "Slonyara",
        .bundle_id = "dev.shekshuev.slonyara",
        .icon_path = "assets/icon.png",
        .default_frame = geometry.RectF.init(0, 0, window_width, window_height),
        .js_window_api = false,
        .security = .{
            .permissions = &app_permissions,
            .navigation = .{ .allowed_origins = &.{ "zero://inline", "zero://app" } },
        },
    }, init);
}

test {
    _ = @import("tests.zig");
}
