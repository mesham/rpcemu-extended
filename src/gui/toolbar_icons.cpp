#include "toolbar_icons.h"

#include <wx/bmpbndl.h>

// SVG icon pack, embedded at build time from src/gui/icons/*.svg. Each entry is
// a NUL-terminated byte array named <basename>_svg (e.g. floppy_svg). See the
// generator in src/gui/CMakeLists.txt.
#include "toolbar_icons_svg.h"

namespace {

// Render an embedded SVG to a bitmap at the requested toolbar size. The SVG is
// authored at a 24x24 reference size; wxBitmapBundle rasterises it cleanly to
// whatever size the toolbar asks for. Used on all platforms for a consistent
// look (wxMSW has no matching wxArtProvider bitmaps; wxGTK theme icons varied).
wxBitmap SvgIcon(const unsigned char *svg, const wxSize &size)
{
	wxBitmapBundle bundle =
	    wxBitmapBundle::FromSVG(reinterpret_cast<const char *>(svg),
	                            wxSize(24, 24));
	return bundle.GetBitmap(size);
}

} // namespace

wxBitmap ToolbarIconScreenshot(const wxSize &size)
{
	return SvgIcon(screenshot_svg, size);
}

wxBitmap ToolbarIconFloppy(const wxSize &size)
{
	return SvgIcon(floppy_svg, size);
}

wxBitmap ToolbarIconCdrom(const wxSize &size)
{
	return SvgIcon(cdrom_svg, size);
}

wxBitmap ToolbarIconReset(const wxSize &size)
{
	return SvgIcon(reset_svg, size);
}

wxBitmap ToolbarIconMute(bool muted, const wxSize &size)
{
	return SvgIcon(muted ? mute_svg : unmute_svg, size);
}

wxBitmap ToolbarIconFullscreen(const wxSize &size)
{
	return SvgIcon(fullscreen_svg, size);
}

wxBitmap ToolbarIconConfigure(const wxSize &size)
{
	return SvgIcon(settings_svg, size);
}

wxBitmap ToolbarIconDebugRun(const wxSize &size)
{
	return SvgIcon(start_svg, size);
}

wxBitmap ToolbarIconDebugPause(const wxSize &size)
{
	return SvgIcon(pause_svg, size);
}

wxBitmap ToolbarIconDebugStep(const wxSize &size)
{
	return SvgIcon(step_svg, size);
}

wxBitmap ToolbarIconInspector(const wxSize &size)
{
	return SvgIcon(inspector_svg, size);
}
