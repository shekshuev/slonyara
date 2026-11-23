const builtin = @import("builtin");
const std = @import("std");

const manifest = @import("app_manifest_zon");
const native_sdk = @import("native_sdk");

const macos = @import("../platform/macos/window_effects.zig");
const windows = @import("../ui/windows/windows.zig");

const is_macos = builtin.os.tag == .macos;

pub const Screen = enum { intro, opening_workspace, workspace };

pub const Model = struct {
    version: []const u8 = manifest.version,
    screen: Screen = .intro,
    appearance: native_sdk.platform.Appearance = .{},
    logo_image: u64 = 0,

    pub const view_unbound = .{ "screen", "appearance" };
};

pub const Msg = union(enum) {
    open_workspace,
    workspace_ready,
    workspace_closed,
    appearance_changed: native_sdk.platform.Appearance,

    pub const view_unbound = .{
        "open_workspace",
        "workspace_ready",
        "workspace_closed",
        "appearance_changed",
    };
};

pub const App = native_sdk.UiApp(Model, Msg);
pub const Effects = native_sdk.Effects(Msg);

fn boot(model: *Model, fx: *Effects) void {
    const logo_id: u64 = 1;
    _ = fx.registerImageBytes(logo_id, @embedFile("app_icon")) catch |err| {
        std.log.warn("Could not load Slonyara icon: {t}", .{err});
        return;
    };
    model.logo_image = logo_id;
}

pub fn update(model: *Model, msg: Msg, fx: *Effects) void {
    switch (msg) {
        .open_workspace => {
            if (model.screen == .intro) model.screen = .opening_workspace;
        },
        .workspace_ready => {
            if (model.screen != .opening_workspace) return;
            model.screen = .workspace;
            fx.hideWindow(windows.intro.label);
        },
        .workspace_closed => {
            model.screen = .intro;
            fx.showWindow(windows.intro.label);
        },
        .appearance_changed => |appearance| model.appearance = appearance,
    }
}

fn onAppearance(appearance: native_sdk.platform.Appearance) ?Msg {
    return .{ .appearance_changed = appearance };
}

fn tokens(model: *const Model) native_sdk.canvas.DesignTokens {
    var theme = native_sdk.canvas.DesignTokens.theme(.{
        .color_scheme = if (model.appearance.color_scheme == .dark) .dark else .light,
        .contrast = if (model.appearance.high_contrast) .high else .standard,
        .reduce_motion = model.appearance.reduce_motion,
    });
    theme.colors.background.a = 0;
    return theme;
}

fn descriptors(model: *const Model, scratch: *App.WindowsScratch) []const App.WindowDescriptor {
    if (model.screen == .intro) return scratch.windows[0..0];
    scratch.windows[0] = windows.main.descriptor(App);
    return scratch.windows[0..1];
}

fn windowView(ui: *App.Ui, model: *const Model, _: []const u8) App.Ui.Node {
    return windows.main.view(Model, Msg, ui, model);
}

pub fn create(allocator: std.mem.Allocator, io: std.Io) !*App {
    return App.create(allocator, .{
        .name = "slonyara",
        .scene = windows.intro.scene,
        .canvas_label = windows.intro.canvas_label,
        .update_fx = update,
        .init_fx = boot,
        .tokens_fn = if (is_macos) tokens else null,
        .on_appearance = if (is_macos) onAppearance else null,
        .windows_fn = descriptors,
        .window_view = windowView,
        .markup = .{
            .source = windows.intro.markup,
            .watch_path = "src/ui/windows/intro_window.native",
            .io = io,
        },
    });
}

pub fn application(ui: *App) native_sdk.App {
    var app = ui.app();
    app.event_fn = event;
    return app;
}

fn event(context: *anyopaque, runtime: *native_sdk.Runtime, value: native_sdk.Event) anyerror!void {
    const ui: *App = @ptrCast(@alignCast(context));
    if (value == .gpu_surface_frame) {
        const frame = value.gpu_surface_frame;
        if (!builtin.is_test and is_macos and std.mem.eql(u8, runtime.options.platform.name, "macos")) {
            macos.configureWindow(frame.label, std.mem.eql(u8, frame.label, windows.intro.canvas_label));
        }
    }
    try ui.app().event(runtime, value);
    if (value == .gpu_surface_frame and
        std.mem.eql(u8, value.gpu_surface_frame.label, windows.main.canvas_label) and
        ui.model.screen == .opening_workspace)
    {
        try ui.dispatch(runtime, value.gpu_surface_frame.window_id, .workspace_ready);
    }
}

pub fn openWorkspace(ui: *App, runtime: *native_sdk.Runtime) !void {
    try ui.dispatch(runtime, 1, .open_workspace);
}
