#include "howm89_cocoa_backend.h"

#if defined(__APPLE__)

#import <Cocoa/Cocoa.h>
#include <string.h>

#ifndef HOWM89_COCOA_MAX_WINDOWS
#define HOWM89_COCOA_MAX_WINDOWS HOWM89_MAX_WINDOWS
#endif

typedef struct HOWM89_CocoaImpl {
    int used;
    NSWindow *window;
    int client_w;
    int client_h;
    int fullscreen;
    int min_w;
    int min_h;
    int max_w;
    int max_h;
} HOWM89_CocoaImpl;

static HOWM89_CocoaImpl g_pool[HOWM89_COCOA_MAX_WINDOWS];

static HOWM89_CocoaImpl *cocoa_alloc(void)
{
    int i;
    for (i = 0; i < HOWM89_COCOA_MAX_WINDOWS; ++i) {
        if (!g_pool[i].used) {
            memset(&g_pool[i], 0, sizeof(g_pool[i]));
            g_pool[i].used = 1;
            return &g_pool[i];
        }
    }
    return 0;
}

static void cocoa_release(HOWM89_CocoaImpl *impl)
{
    if (impl != 0) memset(impl, 0, sizeof(*impl));
}

static int cocoa_init(void *user)
{
    (void)user;
    @autoreleasepool {
        [NSApplication sharedApplication];
        if ([NSApp respondsToSelector:@selector(setActivationPolicy:)]) {
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        }
    }
    return 0;
}

static void cocoa_shutdown(void *user)
{
    (void)user;
}

static void cocoa_measure(HOWM89_CocoaImpl *impl)
{
    if (impl == 0 || impl->window == nil) return;
    @autoreleasepool {
        NSSize s = [[impl->window contentView] bounds].size;
        impl->client_w = (int)s.width;
        impl->client_h = (int)s.height;
    }
}

static void *cocoa_create(void *user, const char *title, int width, int height, int *out_w, int *out_h)
{
    HOWM89_CocoaImpl *impl;
    (void)user;
    if (width <= 0 || height <= 0) return 0;
    impl = cocoa_alloc();
    if (impl == 0) return 0;
    @autoreleasepool {
        NSRect r = NSMakeRect(0, 0, (CGFloat)width, (CGFloat)height);
        NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                           NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
        NSString *s = [NSString stringWithUTF8String:(title ? title : "Howlund Window")];
        impl->window = [[NSWindow alloc] initWithContentRect:r styleMask:style
                                                    backing:NSBackingStoreBuffered defer:NO];
        if (impl->window == nil) {
            cocoa_release(impl);
            return 0;
        }
        [impl->window setTitle:s];
        [impl->window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
    }
    cocoa_measure(impl);
    if (impl->client_w <= 0) impl->client_w = width;
    if (impl->client_h <= 0) impl->client_h = height;
    if (out_w != 0) *out_w = impl->client_w;
    if (out_h != 0) *out_h = impl->client_h;
    return impl;
}

static void cocoa_destroy(void *user, void *ptr)
{
    HOWM89_CocoaImpl *impl = (HOWM89_CocoaImpl *)ptr;
    (void)user;
    if (impl == 0) return;
    @autoreleasepool {
        if (impl->window != nil) {
            [impl->window close];
#if !__has_feature(objc_arc)
            [impl->window release];
#endif
            impl->window = nil;
        }
    }
    cocoa_release(impl);
}

static int cocoa_set_size(void *user, void *ptr, int w, int h)
{
    HOWM89_CocoaImpl *impl=(HOWM89_CocoaImpl*)ptr;(void)user;if(!impl||impl->window==nil||w<=0||h<=0)return -1;
    @autoreleasepool { [impl->window setContentSize:NSMakeSize((CGFloat)w,(CGFloat)h)]; }
    cocoa_measure(impl);return 0;
}
static int cocoa_get_size(void *user, void *ptr, int *w, int *h){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||!w||!h)return -1;cocoa_measure(i);*w=i->client_w;*h=i->client_h;return 0;}
static int cocoa_fullscreen(void *user, void *ptr, int on){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||i->window==nil)return -1;@autoreleasepool{if((on?1:0)!=i->fullscreen){[i->window toggleFullScreen:nil];i->fullscreen=on?1:0;}}return 0;}
static int cocoa_minimize(void *user, void *ptr){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||i->window==nil)return -1;@autoreleasepool{[i->window miniaturize:nil];}return 0;}
static int cocoa_restore(void *user, void *ptr){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||i->window==nil)return -1;@autoreleasepool{[i->window deminiaturize:nil];[i->window makeKeyAndOrderFront:nil];}return 0;}
static int cocoa_maximize(void *user, void *ptr){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||i->window==nil)return -1;@autoreleasepool{[i->window zoom:nil];}return 0;}
static int cocoa_opacity(void *user, void *ptr, HOWM89_Fix q){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||i->window==nil)return -1;if(q<0)q=0;if(q>HOWM89_Q16_ONE)q=HOWM89_Q16_ONE;@autoreleasepool{[i->window setAlphaValue:((CGFloat)q/(CGFloat)HOWM89_Q16_ONE)];}return 0;}
static int cocoa_component(void *user, void *ptr, unsigned int mask, HOWM89_Fix q){(void)mask;return cocoa_opacity(user,ptr,q);}
static void *cocoa_native(void *user, void *ptr){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;return i?(void*)i->window:0;}
static int cocoa_show(void *user, void *ptr, int show){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||i->window==nil)return -1;@autoreleasepool{if(show)[i->window orderFront:nil];else[i->window orderOut:nil];}return 0;}
static int cocoa_visible(void *user, void *ptr, int *out){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||i->window==nil||!out)return -1;@autoreleasepool{*out=[i->window isVisible]?1:0;}return 0;}
static int cocoa_title(void *user, void *ptr, const char *title){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||i->window==nil)return -1;@autoreleasepool{[i->window setTitle:[NSString stringWithUTF8String:(title?title:"")]];}return 0;}
static int cocoa_position(void *user, void *ptr, int x, int y){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||i->window==nil)return -1;@autoreleasepool{NSRect f=[i->window frame];f.origin.x=(CGFloat)x;f.origin.y=(CGFloat)y;[i->window setFrameOrigin:f.origin];}return 0;}
static int cocoa_get_position(void *user, void *ptr, int *x, int *y){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||i->window==nil||!x||!y)return -1;@autoreleasepool{NSRect f=[i->window frame];*x=(int)f.origin.x;*y=(int)f.origin.y;}return 0;}
static int cocoa_resizable(void *user, void *ptr, int value){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;NSUInteger s;(void)user;if(!i||i->window==nil)return -1;@autoreleasepool{s=[i->window styleMask];if(value)s|=NSWindowStyleMaskResizable;else s&=~NSWindowStyleMaskResizable;[i->window setStyleMask:s];}return 0;}
static int cocoa_decorated(void *user, void *ptr, int value){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;NSUInteger s;(void)user;if(!i||i->window==nil)return -1;@autoreleasepool{s=[i->window styleMask];if(value)s|=(NSWindowStyleMaskTitled|NSWindowStyleMaskClosable);else s&=~(NSWindowStyleMaskTitled|NSWindowStyleMaskClosable);[i->window setStyleMask:s];}return 0;}
static int cocoa_topmost(void *user, void *ptr, int value){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||i->window==nil)return -1;@autoreleasepool{[i->window setLevel:value?NSFloatingWindowLevel:NSNormalWindowLevel];}return 0;}
static int cocoa_is_minimized(void *user, void *ptr, int *out){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||i->window==nil||!out)return -1;@autoreleasepool{*out=[i->window isMiniaturized]?1:0;}return 0;}
static int cocoa_is_maximized(void *user, void *ptr, int *out){(void)user;(void)ptr;if(!out)return -1;*out=0;return 0;}
static int cocoa_focus(void *user, void *ptr){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||i->window==nil)return -1;@autoreleasepool{[i->window makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];}return 0;}
static int cocoa_attention(void *user, void *ptr){(void)user;(void)ptr;@autoreleasepool{[NSApp requestUserAttention:NSInformationalRequest];}return 0;}
static int cocoa_close(void *user, void *ptr){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||i->window==nil)return -1;@autoreleasepool{[i->window performClose:nil];}return 0;}
static int cocoa_limits(void *user, void *ptr, int minw,int minh,int maxw,int maxh){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||i->window==nil)return -1;i->min_w=minw;i->min_h=minh;i->max_w=maxw;i->max_h=maxh;@autoreleasepool{if(minw>0&&minh>0)[i->window setContentMinSize:NSMakeSize((CGFloat)minw,(CGFloat)minh)];if(maxw>0&&maxh>0)[i->window setContentMaxSize:NSMakeSize((CGFloat)maxw,(CGFloat)maxh)];}return 0;}
static int cocoa_scale(void *user, void *ptr, HOWM89_Fix *out){HOWM89_CocoaImpl*i=(HOWM89_CocoaImpl*)ptr;(void)user;if(!i||i->window==nil||!out)return -1;@autoreleasepool{CGFloat s=[i->window backingScaleFactor];int milli=(int)(s*(CGFloat)1000);*out=(HOWM89_Fix)((milli*HOWM89_Q16_ONE)/1000);}return 0;}

HOWM89_Result howm89_register_cocoa_backend(void)
{
    HOWM89_BackendVTable v;
    memset(&v,0,sizeof(v));
    v.init=cocoa_init;v.shutdown=cocoa_shutdown;v.create_window=cocoa_create;v.destroy_window=cocoa_destroy;
    v.set_window_size=cocoa_set_size;v.get_window_size=cocoa_get_size;v.set_fullscreen=cocoa_fullscreen;
    v.minimize=cocoa_minimize;v.restore=cocoa_restore;v.maximize=cocoa_maximize;v.set_opacity_q16=cocoa_opacity;
    v.set_component_opacity_q16=cocoa_component;v.get_native_handle=cocoa_native;v.show_window=cocoa_show;
    v.is_window_visible=cocoa_visible;v.set_title=cocoa_title;v.set_position=cocoa_position;v.get_position=cocoa_get_position;
    v.set_resizable=cocoa_resizable;v.set_decorated=cocoa_decorated;v.set_topmost=cocoa_topmost;v.is_minimized=cocoa_is_minimized;
    v.is_maximized=cocoa_is_maximized;v.focus=cocoa_focus;v.request_attention=cocoa_attention;v.request_close=cocoa_close;
    v.set_size_limits=cocoa_limits;v.get_scale_factor_q16=cocoa_scale;
    return howm89_register_backend(&v,0);
}

#else
HOWM89_Result howm89_register_cocoa_backend(void){return HOWM89_ERROR_NOT_SUPPORTED;}
#endif
