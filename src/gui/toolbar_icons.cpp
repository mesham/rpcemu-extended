#include "toolbar_icons.h"

#include <wx/artprov.h>
#include <wx/image.h>

#if defined(__WXGTK__)
#include <gtk/gtk.h>
#endif

namespace {

wxBitmap ScaleBitmap(const wxBitmap &source, const wxSize &size)
{
	if (!source.IsOk()) {
		return wxNullBitmap;
	}

	wxImage image = source.ConvertToImage();
	if (!image.IsOk()) {
		return source;
	}

	if (!image.HasAlpha() && image.HasMask()) {
		image.InitAlpha();
	}

	if (image.GetWidth() == size.GetWidth() && image.GetHeight() == size.GetHeight()) {
		return wxBitmap(image);
	}

	image.Rescale(size.GetWidth(), size.GetHeight(), wxIMAGE_QUALITY_HIGH);
	return wxBitmap(image);
}

wxBitmap EmbeddedIcon(const char *const *xpm)
{
	wxImage image(xpm);
	if (!image.IsOk()) {
		return wxNullBitmap;
	}

	if (!image.HasAlpha() && image.HasMask()) {
		image.InitAlpha();
	}

	return wxBitmap(image);
}

#if defined(__WXGTK__)
wxImage GdkPixbufToWxImage(GdkPixbuf *pixbuf)
{
	const int width = gdk_pixbuf_get_width(pixbuf);
	const int height = gdk_pixbuf_get_height(pixbuf);
	const bool has_alpha = gdk_pixbuf_get_has_alpha(pixbuf) != 0;
	const int stride = gdk_pixbuf_get_rowstride(pixbuf);
	const guchar *pixels = gdk_pixbuf_get_pixels(pixbuf);
	const int channels = has_alpha ? 4 : 3;

	wxImage image(width, height);
	unsigned char *rgb = image.GetData();
	unsigned char *alpha = nullptr;
	if (has_alpha) {
		image.InitAlpha();
		alpha = image.GetAlpha();
	}

	for (int y = 0; y < height; ++y) {
		const guchar *row = pixels + y * stride;
		for (int x = 0; x < width; ++x) {
			const guchar *pixel = row + x * channels;
			const int offset = (y * width + x) * 3;
			rgb[offset] = pixel[0];
			rgb[offset + 1] = pixel[1];
			rgb[offset + 2] = pixel[2];
			if (alpha != nullptr) {
				alpha[y * width + x] = pixel[3];
			}
		}
	}

	return image;
}

wxBitmap GtkThemeIcon(const char *const *names, size_t name_count, const wxSize &size)
{
	GtkIconTheme *theme = gtk_icon_theme_get_default();
	if (theme == nullptr) {
		return wxNullBitmap;
	}

	for (size_t i = 0; i < name_count; ++i) {
		GError *error = nullptr;
		GdkPixbuf *pixbuf = gtk_icon_theme_load_icon(
		    theme, names[i], size.GetWidth(), GTK_ICON_LOOKUP_USE_BUILTIN, &error);
		if (error != nullptr) {
			g_error_free(error);
		}
		if (pixbuf == nullptr) {
			continue;
		}

		wxImage image = GdkPixbufToWxImage(pixbuf);
		g_object_unref(pixbuf);
		if (!image.IsOk()) {
			continue;
		}

		return wxBitmap(image);
	}

	return wxNullBitmap;
}
#endif

wxBitmap WxArtIcon(const char *art_id, const wxSize &size)
{
	return wxArtProvider::GetBitmap(art_id, wxART_TOOLBAR, size);
}

wxBitmap ResolveIcon(const char *const *theme_names,
                     size_t theme_name_count,
                     const char *wx_art_id,
                     const wxBitmap &embedded,
                     const wxSize &size)
{
#if defined(__WXGTK__)
	wxBitmap gtk_icon = GtkThemeIcon(theme_names, theme_name_count, size);
	if (gtk_icon.IsOk()) {
		return gtk_icon;
	}
#endif

	if (embedded.IsOk()) {
		return ScaleBitmap(embedded, size);
	}

	return WxArtIcon(wx_art_id, size);
}

static const char *kCameraXpm[] = {
    "24 24 4 1",
    "  c None",
    ". c #000000",
    "+ c #4d4d4d",
    "o c #ffffff",
    "                        ",
    "      ++++++++++        ",
    "    ++oooooooooo++      ",
    "   +oo++++++++++oo+     ",
    "  +oo++........++oo+    ",
    "  +oo++........++oo+    ",
    " +oo++..........++oo+   ",
    " +oo++..........++oo+   ",
    " +oo++..........++oo+   ",
    " +oo++..........++oo+   ",
    " +oo++..........++oo+   ",
    " +oo++..........++oo+   ",
    "  +oo++........++oo+    ",
    "  +oo++........++oo+    ",
    "   +oo++++++++++oo+     ",
    "    ++oooooooooo++      ",
    "      ++++++++++        ",
    "         +++++          ",
    "         +++++          ",
    "         +++++          ",
    "         +++++          ",
    "         +++++          ",
    "                        ",
    "                        ",
};

static const char *kSpeakerXpm[] = {
    "24 24 4 1",
    "  c None",
    ". c #000000",
    "+ c #4d4d4d",
    "o c #ffffff",
    "                        ",
    "                        ",
    "      ++++++            ",
    "     +oooooo+           ",
    "    +oooooooo+          ",
    "   +oooooooooo+         ",
    "   +oooooooooo+         ",
    "   +oooooooooo+         ",
    "   +oooooooooo+         ",
    "   +oooooooooo+         ",
    "    +oooooooo+          ",
    "     +oooooo+           ",
    "      ++++++            ",
    "                        ",
    "                        ",
    "                        ",
    "                        ",
    "                        ",
    "                        ",
    "                        ",
    "                        ",
    "                        ",
    "                        ",
    "                        ",
};

static const char *kSpeakerMutedXpm[] = {
    "24 24 4 1",
    "  c None",
    ". c #000000",
    "+ c #4d4d4d",
    "o c #ffffff",
    "                        ",
    "                        ",
    "      ++++++            ",
    "     +oooooo+           ",
    "    +oooooooo+          ",
    "   +oooooooooo+         ",
    "   +oooooooooo+         ",
    "   +oooooooooo+         ",
    "   +oooooooooo+         ",
    "   +oooooooooo+         ",
    "    +oooooooo+          ",
    "     +oooooo+           ",
    "      ++++++            ",
    "           ++           ",
    "            ++          ",
    "             ++         ",
    "              ++        ",
    "               ++       ",
    "                ++      ",
    "                 ++     ",
    "                  ++    ",
    "                   ++   ",
    "                        ",
    "                        ",
};

} // namespace

wxBitmap ToolbarIconScreenshot(const wxSize &size)
{
	static const char *names[] = {"camera-photo", "applets-screenshooter", "camera"};
	static const wxBitmap embedded = EmbeddedIcon(kCameraXpm);
	return ResolveIcon(names, WXSIZEOF(names), wxART_PRINT, embedded, size);
}

wxBitmap ToolbarIconFloppy(const wxSize &size)
{
	static const char *names[] = {"media-floppy", "drive-removable-media"};
	return ResolveIcon(names, WXSIZEOF(names), wxART_FLOPPY, wxNullBitmap, size);
}

wxBitmap ToolbarIconCdrom(const wxSize &size)
{
	static const char *names[] = {"media-optical", "media-cdrom", "drive-optical"};
	return ResolveIcon(names, WXSIZEOF(names), wxART_CDROM, wxNullBitmap, size);
}

wxBitmap ToolbarIconReset(const wxSize &size)
{
	static const char *names[] = {"view-refresh", "system-reboot", "reload"};
	return ResolveIcon(names, WXSIZEOF(names), wxART_REFRESH, wxNullBitmap, size);
}

wxBitmap ToolbarIconMute(bool muted, const wxSize &size)
{
	if (muted) {
		static const char *names[] = {"audio-volume-muted", "audio-volume-off"};
		static const wxBitmap embedded = EmbeddedIcon(kSpeakerMutedXpm);
		return ResolveIcon(names, WXSIZEOF(names), wxART_ERROR, embedded, size);
	}

	static const char *names[] = {"audio-volume-high", "speaker", "audio-volume-medium"};
	static const wxBitmap embedded = EmbeddedIcon(kSpeakerXpm);
	return ResolveIcon(names, WXSIZEOF(names), wxART_ERROR, embedded, size);
}

wxBitmap ToolbarIconFullscreen(const wxSize &size)
{
	static const char *names[] = {"view-fullscreen", "fullscreen"};
	return ResolveIcon(names, WXSIZEOF(names), wxART_FULL_SCREEN, wxNullBitmap, size);
}

wxBitmap ToolbarIconConfigure(const wxSize &size)
{
	static const char *names[] = {"preferences-system", "gtk-preferences", "emblem-system"};
	return ResolveIcon(names, WXSIZEOF(names), wxART_HELP_SETTINGS, wxNullBitmap, size);
}

wxBitmap ToolbarIconDebugRun(const wxSize &size)
{
	static const char *names[] = {"media-playback-start", "media-play"};
	return ResolveIcon(names, WXSIZEOF(names), wxART_GO_FORWARD, wxNullBitmap, size);
}

wxBitmap ToolbarIconDebugPause(const wxSize &size)
{
	static const char *names[] = {"media-playback-pause", "media-pause"};
	return ResolveIcon(names, WXSIZEOF(names), wxART_STOP, wxNullBitmap, size);
}

wxBitmap ToolbarIconDebugStep(const wxSize &size)
{
	static const char *names[] = {"media-skip-forward", "go-next"};
	return ResolveIcon(names, WXSIZEOF(names), wxART_GO_TO_PARENT, wxNullBitmap, size);
}

wxBitmap ToolbarIconInspector(const wxSize &size)
{
	static const char *names[] = {"computer", "system-run", "utilities-system-monitor"};
	return ResolveIcon(names, WXSIZEOF(names), wxART_EXECUTABLE_FILE, wxNullBitmap, size);
}
