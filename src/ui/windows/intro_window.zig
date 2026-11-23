const builtin = @import("builtin");
const std = @import("std");

const manifest = @import("app_manifest_zon");
const native_sdk = @import("native_sdk");

const controller = @import("../../app/controller.zig");

const testing = std.testing;
const canvas = native_sdk.canvas;

pub const label = "intro";
pub const canvas_label = "intro-canvas";
pub const width: f32 = 640;
pub const height: f32 = 480;
pub const markup = @embedFile("intro_window.native");
const is_macos = builtin.os.tag == .macos;

const views = [_]native_sdk.ShellView{.{
    .label = canvas_label,
    .kind = .gpu_surface,
    .fill = true,
    .role = "Connection picker",
    .accessibility_label = "Connections",
    .gpu_backend = .metal,
    .gpu_pixel_format = .bgra8_unorm,
    .gpu_present_mode = .timer,
    .gpu_alpha_mode = if (is_macos) .premultiplied else .@"opaque",
    .gpu_color_space = .srgb,
    .gpu_vsync = true,
}};
const window_configs = [_]native_sdk.ShellWindow{.{
    .label = label,
    .title = "Slonyara",
    .width = width,
    .height = height,
    .resizable = false,
    .restore_state = false,
    .allows_fullscreen = false,
    .transparent = is_macos,
    .views = &views,
}};
pub const scene: native_sdk.ShellConfig = .{ .windows = &window_configs };

const size = native_sdk.geometry.SizeF.init(640, 480);

fn decodeIcon(context: ?*anyopaque, bytes: []const u8, buffer: []u8, _: usize) !native_sdk.platform.DecodedImage {
    const platform: *native_sdk.platform.NullPlatform = @ptrCast(@alignCast(context.?));
    platform.image_decode_count += 1;
    try testing.expectEqualSlices(u8, @embedFile("app_icon"), bytes);
    if (buffer.len < 4) return error.ImageTooLarge;
    @memcpy(buffer[0..4], &[_]u8{ 0, 128, 255, 255 });
    return .{ .width = 1, .height = 1, .rgba8 = buffer[0..4] };
}

const Fixture = struct {
    harness: *native_sdk.TestHarness(),
    ui: *controller.App,

    fn create() !Fixture {
        const harness = try native_sdk.TestHarness().create(testing.allocator, .{ .size = size });
        errdefer harness.destroy(testing.allocator);
        harness.null_platform.gpu_surfaces = true;
        harness.runtime.options.platform.services.decode_image_fn = decodeIcon;
        _ = try harness.runtime.options.platform.services.createWindow(.{
            .id = 1,
            .label = label,
            .title = "Slonyara",
            .default_frame = native_sdk.geometry.RectF.fromSize(size),
            .resizable = false,
            .restore_state = false,
            .allows_fullscreen = false,
        });
        const ui = try controller.create(testing.allocator, testing.io);
        errdefer ui.destroy();
        var fixture = Fixture{ .harness = harness, .ui = ui };
        try harness.start(controller.application(ui));
        errdefer harness.stop(controller.application(ui)) catch {};
        try fixture.frame(1);
        return fixture;
    }

    fn deinit(self: *Fixture) void {
        self.harness.stop(controller.application(self.ui)) catch {};
        self.ui.destroy();
        self.harness.destroy(testing.allocator);
    }

    fn frame(self: *Fixture, index: u64) !void {
        try self.harness.runtime.dispatchPlatformEvent(controller.application(self.ui), .{ .gpu_surface_frame = .{
            .window_id = 1,
            .label = canvas_label,
            .size = size,
            .frame_index = index,
        } });
    }
};

fn find(widget: canvas.Widget, kind: canvas.WidgetKind, text: []const u8) !canvas.Widget {
    if (widget.kind == kind and std.mem.eql(u8, widget.text, text)) return widget;
    for (widget.children) |child| {
        const found = find(child, kind, text) catch continue;
        return found;
    }
    return error.WidgetNotFound;
}

fn buildTestTree(arena: std.mem.Allocator, model: *const controller.Model) !controller.App.Ui.Tree {
    var view = try canvas.MarkupView(controller.Model, controller.Msg).init(arena, markup);
    var ui = controller.App.Ui.init(arena);
    return ui.finalize(try view.build(&ui, model));
}

test "Intro starts as the only window with fixed 640x480 geometry" {
    var fixture = try Fixture.create();
    defer fixture.deinit();
    var storage: [8]native_sdk.platform.WindowInfo = undefined;
    const open = fixture.harness.runtime.listWindows(&storage);
    try testing.expectEqual(@as(usize, 1), open.len);
    try testing.expectEqualStrings("intro", open[0].label);
    try testing.expect(open[0].open and !open[0].hidden);
    try testing.expectEqual(@as(f32, 640), open[0].frame.width);
    try testing.expectEqual(@as(f32, 480), open[0].frame.height);
    try testing.expectEqual(controller.Screen.intro, fixture.ui.model.screen);
    const config = scene.windows[0];
    try testing.expect(!config.resizable and !config.restore_state and !config.allows_fullscreen);
    const startup = manifest.shell.windows[0];
    try testing.expectEqual(config.width, startup.width);
    try testing.expectEqual(config.height, startup.height);
    try testing.expect(!startup.resizable and !startup.restore_state and !startup.allows_fullscreen);
}

test "Intro shows its initial empty state and buttons have no action bindings" {
    var fixture = try Fixture.create();
    defer fixture.deinit();
    const tree = fixture.ui.tree.?;
    _ = try find(tree.root, .text, "SLONYARA");
    _ = try find(tree.root, .text, "No connections yet");
    const search = try find(tree.root, .search_field, "");
    try testing.expectEqualStrings("Search connections…", search.placeholder);
    const list = try find(tree.root, .list, "");
    try testing.expectEqual(@as(usize, 0), list.children.len);
    const connect = try find(tree.root, .button, "Connect");
    const edit = try find(tree.root, .button, "Edit");
    const create = try find(tree.root, .button, "New Connection");
    try testing.expect(connect.state.disabled and edit.state.disabled);
    try testing.expect(!create.state.disabled);
    try testing.expect(tree.msgForPointer(create.id, .up) == null);
}

test "Intro version comes from the manifest and renders as a binding" {
    var arena = std.heap.ArenaAllocator.init(testing.allocator);
    defer arena.deinit();
    var model = controller.Model{};
    try testing.expectEqualStrings(manifest.version, model.version);
    const tree = try buildTestTree(arena.allocator(), &model);
    const expected = try std.fmt.allocPrint(arena.allocator(), "Version: {s}", .{manifest.version});
    _ = try find(tree.root, .text, expected);
    model.version = "9.8.7-test";
    const changed = try buildTestTree(arena.allocator(), &model);
    _ = try find(changed.root, .text, "Version: 9.8.7-test");
}

test "Intro registers the embedded icon once and binds its registered image" {
    var fixture = try Fixture.create();
    defer fixture.deinit();
    try testing.expect(fixture.ui.model.logo_image != 0);
    const image = fixture.harness.runtime.registeredCanvasImage(fixture.ui.model.logo_image) orelse return error.LogoNotRegistered;
    try testing.expect(image.width > 0 and image.height > 0);
    const logo = try find(fixture.ui.tree.?.root, .image, "");
    try testing.expectEqual(fixture.ui.model.logo_image, logo.image_id);
    try testing.expectEqualStrings("Slonyara logo", logo.semantics.label);
    try testing.expectEqual(@as(usize, 1), fixture.harness.runtime.registeredCanvasImageCount());
    const decodes = fixture.harness.null_platform.image_decode_count;
    try fixture.frame(2);
    try fixture.frame(3);
    try testing.expectEqual(decodes, fixture.harness.null_platform.image_decode_count);
    try testing.expectEqual(@as(usize, 1), fixture.harness.runtime.registeredCanvasImageCount());
}

test "Intro keeps its content and logo when system appearance changes" {
    var fixture = try Fixture.create();
    defer fixture.deinit();
    const logo = fixture.ui.model.logo_image;
    for ([_]native_sdk.platform.Appearance{
        .{ .color_scheme = .dark },
        .{ .color_scheme = .light },
        .{ .color_scheme = .dark, .high_contrast = true, .reduce_motion = true },
    }) |appearance| {
        try fixture.harness.runtime.dispatchPlatformEvent(controller.application(fixture.ui), .{ .appearance_changed = appearance });
        if (is_macos) try testing.expectEqual(appearance, fixture.ui.model.appearance);
        try testing.expectEqual(controller.Screen.intro, fixture.ui.model.screen);
        try testing.expectEqual(logo, fixture.ui.model.logo_image);
        _ = try find(fixture.ui.tree.?.root, .button, "New Connection");
        _ = try find(fixture.ui.tree.?.root, .text, "No connections yet");
    }
}

test "Intro content fits its fixed size in light and dark themes" {
    var arena = std.heap.ArenaAllocator.init(testing.allocator);
    defer arena.deinit();
    const model = controller.Model{};
    const tree = try buildTestTree(arena.allocator(), &model);
    for ([_]canvas.ColorScheme{ .light, .dark }) |scheme| {
        try canvas.expectLayoutAuditSweepClean(testing.allocator, tree.root, .{
            .tokens = canvas.DesignTokens.theme(.{ .color_scheme = scheme }),
            .min_size = size,
            .default_size = size,
            .large_size = size,
            .densities = &.{.regular},
            .text_expansions = &.{1},
        });
    }
}

test "Closing Intro leaves no live windows and does not recreate it" {
    var fixture = try Fixture.create();
    defer fixture.deinit();
    const app = controller.application(fixture.ui);
    try fixture.harness.runtime.dispatchPlatformEvent(app, .{ .window_frame_changed = .{
        .id = 1,
        .label = label,
        .frame = native_sdk.geometry.RectF.fromSize(size),
        .open = false,
    } });
    try app.event(&fixture.harness.runtime, .{ .lifecycle = .frame });
    var storage: [8]native_sdk.platform.WindowInfo = undefined;
    for (fixture.harness.runtime.listWindows(&storage)) |window| {
        try testing.expect(!window.open);
    }
}
