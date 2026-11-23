#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#import <objc/runtime.h>
#include <stdbool.h>

static NSView *findCanvas(NSView *view, NSString *label) {
    if ([view.identifier isEqualToString:label]) return view;
    for (NSView *child in view.subviews) {
        NSView *found = findCanvas(child, label);
        if (found) return found;
    }
    return nil;
}

void configure_window(const char *label, size_t label_len, bool intro) {
    NSCAssert(NSThread.isMainThread, @"Window effects must run on the AppKit thread");
    static char materialKey;
    NSString *identifier = [[NSString alloc] initWithBytes:label length:label_len
                                                encoding:NSUTF8StringEncoding];
    for (NSWindow *window in NSApp.windows) {
        NSView *canvas = findCanvas(window.contentView, identifier);
        if (!canvas) continue;
        NSVisualEffectView *existing = objc_getAssociatedObject(canvas, &materialKey);
        if (existing.superview == window.contentView) return;

        NSView *content = window.contentView;
        NSVisualEffectView *material = [[NSVisualEffectView alloc] initWithFrame:content.bounds];
        material.wantsLayer = YES;
        material.material = NSVisualEffectMaterialUnderWindowBackground;
        material.blendingMode = NSVisualEffectBlendingModeBehindWindow;
        material.state = NSVisualEffectStateFollowsWindowActiveState;
        material.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        [content addSubview:material positioned:NSWindowBelow relativeTo:nil];
        objc_setAssociatedObject(canvas, &materialKey, material, OBJC_ASSOCIATION_RETAIN_NONATOMIC);

        window.opaque = NO;
        window.backgroundColor = NSColor.clearColor;
        window.titlebarAppearsTransparent = YES;
        window.styleMask |= NSWindowStyleMaskFullSizeContentView;
        if (intro) {
            window.titleVisibility = NSWindowTitleHidden;
            window.styleMask &= ~(NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable);
            [window standardWindowButton:NSWindowMiniaturizeButton].hidden = YES;
            [window standardWindowButton:NSWindowZoomButton].hidden = YES;
        }

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
