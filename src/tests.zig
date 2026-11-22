const std = @import("std");
const native_sdk = @import("native_sdk");
const main = @import("main.zig");

const canvas = native_sdk.canvas;

test "the window content is empty" {
    var arena_state = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena_state.deinit();
    const arena = arena_state.allocator();

    var view = try canvas.MarkupView(main.Model, main.Msg).init(arena, main.app_markup);
    var ui = main.AppUi.init(arena);
    const model = main.Model{};
    const tree = try ui.finalize(try view.build(&ui, &model));

    try std.testing.expectEqual(@as(usize, 0), tree.root.children.len);
    try std.testing.expectEqualStrings("", tree.root.text);
}
