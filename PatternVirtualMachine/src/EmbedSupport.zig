const wrtl = @import("WolframRTL.zig");
const Expr = @import("Expr.zig");

const std = @import("std");

pub const Embeddable = struct {
    ptr: *anyopaque,
    vtable: *const VTable,

    const VTable = struct {
        getEmbedName: *const fn () [:0]const u8,
        initializeEmbedMethods: *const fn () void,
        deleteMethod: *const fn (ptr: *anyopaque) void,
    };

    pub fn init(ptr: anytype) Embeddable {
        const T = @TypeOf(ptr);
        const ptr_info = @typeInfo(T);

        if (ptr_info != .Pointer or ptr_info.Pointer.size != .one) {
            @compileError("ptr must be a single-item pointer");
        }

        const impl = struct {
            fn getEmbedName() [:0]const u8 {
                return @field(ptr_info.Pointer.child, "embed_name");
            }

            fn initializeEmbedMethods() void {
                T.initializeEmbedMethods();
            }

            fn deleteMethod(pointer: *anyopaque) void {
                const self: T = @ptrCast(@alignCast(pointer));
                if (@sizeOf(T) > 0) {
                    self.deinit();
                }
            }
        };

        return .{
            .ptr = ptr,
            .vtable = &.{
                .embedName = impl.getEmbedName,
                .initializeEmbedMethods = impl.initializeEmbedMethods,
                .deleteMethod = impl.deleteMethod,
            },
        };
    }

    pub fn setupEmbed(self: Embeddable) void {
        const Initialized = struct {
            var initialized: bool = false;
        };

        const embedName = self.getEmbedName();
        if (!Initialized.initialized) {
            wrtl.InitializeCompilerClass_Export(embedName);
            self.vtable.initializeEmbedMethods();
            wrtl.FinalizeCompilerClass_Export(embedName);
            Initialized.initialized = true;
        }
    }

    pub fn embedObject(self: Embeddable) Expr {
        self.setupEmbed();
        const embedName = self.getEmbedName();
        const head = Expr.inertExpression(embedName);
        return Expr.embedObjectInstance(self.ptr, embedName, head);
    }

    pub fn setDeleteMethod(self: Embeddable, embedName: [:0]const u8) void {
        _ = wrtl.SetClassRawMethod(
            embedName,
            "releaseInstance",
            @ptrCast(&self.vtable.deleteMethod),
        );
    }

    pub fn getEmbedName(self: Embeddable) [:0]const u8 {
        return self.vtable.embedName();
    }
};

/// Embed an object instance into an Expression
pub fn embedObject(comptime T: type, inst: *T) Expr {
    const embeddable = Embeddable.init(inst);
    const embedName = embeddable.getEmbedName();
    return embeddable.embedObject(embedName);
}

/// Embed a shared object (allocates new storage for the pointer)
pub fn embedObjectShared(comptime T: type, allocator: std.mem.Allocator, obj: *T) !Expr {
    const boxed = try allocator.create(*T);
    boxed.* = obj;

    const embeddable = Embeddable.init(boxed);
    const embedName = embeddable.getEmbedName();
    embeddable.setDeleteMethod(embedName);
    return embeddable.embedObject(embedName);
}

/// Extract an embedded object from an Expr
pub fn unembedObject(comptime T: type, self: Expr) ?*T {
    const embedName = T.EmbedName();
    if (Expr.unembedObjectInstance(self, embedName)) |obj| {
        return @ptrCast(@alignCast(obj));
    }
    return null;
}

/// Extract a shared object from an Expression
pub fn unembedObjectShared(comptime T: type, self: Expr) ?*T {
    if (Expr.unembedObjectInstance(self, T.embed_name)) |obj| {
        const boxed: **T = @ptrCast(@alignCast(obj));
        return boxed.*;
    }
    return null;
}
