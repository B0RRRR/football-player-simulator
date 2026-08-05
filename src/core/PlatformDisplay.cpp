#include "PlatformDisplay.h"

#if defined(__linux__)
#include <X11/Xlib.h>

// RandR 1.5 monitor API declared here so we only need the X11 headers (present) plus the
// libXrandr runtime lib - not the libXrandr -dev headers. Layout matches <X11/extensions/Xrandr.h>.
extern "C" {
    typedef struct {
        Atom name;
        Bool primary;
        Bool automatic;
        int noutput;
        int x, y, width, height;
        int mwidth, mheight;
        XID* outputs;
    } XRRMonitorInfo;
    XRRMonitorInfo* XRRGetMonitors(Display* dpy, Window window, Bool get_active, int* nmonitors);
    void XRRFreeMonitors(XRRMonitorInfo* monitors);
}

namespace Platform {
bool monitorContaining(int px, int py, int& ox, int& oy, int& ow, int& oh) {
    Display* d = XOpenDisplay(nullptr);
    if (!d) return false;
    int n = 0;
    bool found = false;
    // XDefaultRootWindow (the function, not the DefaultRootWindow macro) avoids expanding to
    // code that references Xlib's `Screen` type.
    XRRMonitorInfo* mons = XRRGetMonitors(d, XDefaultRootWindow(d), 1 /*active*/, &n);
    if (mons && n > 0) {
        int pick = -1;
        for (int i = 0; i < n; ++i)
            if (px >= mons[i].x && px < mons[i].x + mons[i].width &&
                py >= mons[i].y && py < mons[i].y + mons[i].height) { pick = i; break; }
        if (pick < 0) pick = 0; // point off-screen -> first/primary monitor
        ox = mons[pick].x; oy = mons[pick].y; ow = mons[pick].width; oh = mons[pick].height;
        found = true;
    }
    if (mons) XRRFreeMonitors(mons);
    XCloseDisplay(d);
    return found;
}
}

#else  // non-Linux: no query; callers fall back to the whole desktop.
namespace Platform {
bool monitorContaining(int, int, int&, int&, int&, int&) { return false; }
}
#endif
