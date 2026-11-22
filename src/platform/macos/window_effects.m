#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>

static NSView *findCanvas(NSView *view, NSString *label) {
    if ([view.identifier isEqualToString:label]) return view;
    for (NSView *child in view.subviews) {
        NSView *found = findCanvas(child, label);
        if (found) return found;
    }
    return nil;
}

void apply_vibrancy(const char *label, size_t label_len) {
    NSCAssert(NSThread.isMainThread, @"Window effects must run on the AppKit thread");
    static __weak NSView *canvas;
    static __weak NSVisualEffectView *effect;

    if (!canvas.window || !effect.superview) {
        NSString *identifier = [[NSString alloc] initWithBytesNoCopy:(void *)label
                                                              length:label_len
                                                            encoding:NSUTF8StringEncoding
                                                        freeWhenDone:NO];
        for (NSWindow *window in NSApp.windows) {
            NSView *candidate = findCanvas(window.contentView, identifier);
            if (!candidate) continue;
            canvas = candidate;

            NSView *content = window.contentView;

            NSVisualEffectView *material = [[NSVisualEffectView alloc] initWithFrame:content.bounds];
            material.wantsLayer = YES;
            material.material = NSVisualEffectMaterialUnderWindowBackground;
            material.blendingMode = NSVisualEffectBlendingModeBehindWindow;
            material.state = NSVisualEffectStateFollowsWindowActiveState;
            material.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;

            [content addSubview:material positioned:NSWindowBelow relativeTo:nil];
            effect = material;

            window.opaque = NO;
            window.backgroundColor = NSColor.clearColor;
            window.titlebarAppearsTransparent = YES;
            window.styleMask |= NSWindowStyleMaskFullSizeContentView;

            content.wantsLayer = YES;
            content.layer.opaque = NO;
            content.layer.backgroundColor = NSColor.clearColor.CGColor;

            canvas.wantsLayer = YES;
            canvas.layer.opaque = NO;
            canvas.layer.backgroundColor = NSColor.clearColor.CGColor;
            for (CALayer *sublayer in canvas.layer.sublayers) {
                sublayer.opaque = NO;
            }
            break;
        }
    }
}