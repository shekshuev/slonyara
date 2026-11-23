const builtin = @import("builtin");

const native_sdk = @import("native_sdk");

pub const label = "main";
pub const canvas_label = "main-canvas";
pub const width: f32 = 1200;
pub const height: f32 = 750;
pub const markup = @embedFile("main_window.native");
const is_macos = builtin.os.tag == .macos;

pub fn descriptor(comptime App: type) App.WindowDescriptor {
    return .{
        .label = label,
        .canvas_label = canvas_label,
        .title = "Slonyara",
        .width = width,
        .height = height,
        .transparent = is_macos,
        .on_close = .workspace_closed,
    };
}

pub fn view(comptime Model: type, comptime Msg: type, ui: *native_sdk.canvas.Ui(Msg), model: *const Model) native_sdk.canvas.Ui(Msg).Node {
    return native_sdk.canvas.CompiledMarkupView(Model, Msg, markup).build(ui, model);
}
