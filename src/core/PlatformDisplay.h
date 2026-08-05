#pragma once

// Tiny platform hook for multi-monitor geometry, kept in its own translation unit so the X11
// headers (which define clashing `Screen`/`None` identifiers) never mix with the rest of the
// codebase. No platform types leak through this interface.
namespace Platform {
    // Fills the bounds of the physical monitor that contains the point (px, py). Returns false
    // when it can't be determined (non-Linux build, or the query failed) - callers then fall
    // back to the whole-desktop size.
    bool monitorContaining(int px, int py, int& x, int& y, int& w, int& h);
}
