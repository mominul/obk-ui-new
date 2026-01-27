#include <QtGlobal>

#ifdef Q_OS_MAC

#import <Cocoa/Cocoa.h>
#import <AppKit/AppKit.h>

extern "C" {

void enableMacOSBlur(void* winId)
{
    if (!winId) return;

    @autoreleasepool {
        NSView* nsView = (__bridge NSView*)(void*)(intptr_t)winId;
        if (!nsView) return;

        NSWindow* window = [nsView window];
        if (!window) return;

        // Basic window setup for transparency - no NSVisualEffectView
        // as it interferes with Qt widget rendering
        [window setOpaque:NO];
        [window setBackgroundColor:[NSColor clearColor]];
        [window setHasShadow:YES];

        // Set dark appearance
        if (@available(macOS 10.14, *)) {
            [window setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];
        }
    }
}

} // extern "C"

#endif // Q_OS_MAC
