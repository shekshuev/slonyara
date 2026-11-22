const builtin = @import("builtin");

extern "c" fn apply_vibrancy(label: [*]const u8, label_len: usize) void;

pub fn applyVibrancy(canvas_label: []const u8) void {
    if (comptime builtin.os.tag == .macos) {
        apply_vibrancy(canvas_label.ptr, canvas_label.len);
    }
}
