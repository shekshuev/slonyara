const builtin = @import("builtin");

extern "c" fn configure_window(label: [*]const u8, label_len: usize, intro: bool) void;

pub fn configureWindow(canvas_label: []const u8, intro: bool) void {
    if (comptime builtin.os.tag == .macos) {
        configure_window(canvas_label.ptr, canvas_label.len, intro);
    }
}
