#ifndef TOOLBAR_ICONS_H
#define TOOLBAR_ICONS_H

#include <wx/bitmap.h>
#include <wx/gdicmn.h>

wxBitmap ToolbarIconScreenshot(const wxSize &size = wxSize(24, 24));
wxBitmap ToolbarIconFloppy(const wxSize &size = wxSize(24, 24));
wxBitmap ToolbarIconCdrom(const wxSize &size = wxSize(24, 24));
wxBitmap ToolbarIconReset(const wxSize &size = wxSize(24, 24));
wxBitmap ToolbarIconMute(bool muted, const wxSize &size = wxSize(24, 24));
wxBitmap ToolbarIconFullscreen(const wxSize &size = wxSize(24, 24));
wxBitmap ToolbarIconConfigure(const wxSize &size = wxSize(24, 24));
wxBitmap ToolbarIconDebugRun(const wxSize &size = wxSize(24, 24));
wxBitmap ToolbarIconDebugPause(const wxSize &size = wxSize(24, 24));
wxBitmap ToolbarIconDebugStep(const wxSize &size = wxSize(24, 24));
wxBitmap ToolbarIconInspector(const wxSize &size = wxSize(24, 24));

#endif
