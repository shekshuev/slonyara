pub const client = @import("postgres/client.zig");
pub const oids = @import("postgres/oids.zig");
pub const options = @import("postgres/option.zig");
pub const result = @import("postgres/result.zig");

test {
    _ = client;
    _ = oids;
    _ = result;
    _ = options;
}
