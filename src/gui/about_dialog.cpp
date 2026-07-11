#include "about_dialog.h"

#include <algorithm>

#include <wx/artprov.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/statline.h>

extern "C" {
#include "rpcemu.h"
}

namespace {

wxBitmap
CreateAboutIcon(int size)
{
	wxImage image(size, size);
	image.InitAlpha();

	const wxColour outer(0x2C, 0x5F, 0x8A);
	const wxColour inner(0xF0, 0xE8, 0xD0);
	const int margin = std::max(1, size / 8);
	const int radius = std::max(1, size / 10);

	auto inside_rounded_rect = [&](int x, int y) {
		const int left = margin;
		const int top = margin;
		const int right = size - margin - 1;
		const int bottom = size - margin - 1;

		if (x < left || x > right || y < top || y > bottom) {
			return false;
		}

		auto inside_corner = [&](int cx, int cy) {
			const int dx = x - cx;
			const int dy = y - cy;
			return (dx * dx + dy * dy) <= (radius * radius);
		};

		if (x < left + radius && y < top + radius) {
			return inside_corner(left + radius, top + radius);
		}
		if (x > right - radius && y < top + radius) {
			return inside_corner(right - radius, top + radius);
		}
		if (x < left + radius && y > bottom - radius) {
			return inside_corner(left + radius, bottom - radius);
		}
		if (x > right - radius && y > bottom - radius) {
			return inside_corner(right - radius, bottom - radius);
		}
		return true;
	};

	for (int y = 0; y < size; ++y) {
		for (int x = 0; x < size; ++x) {
			const wxColour &colour = inside_rounded_rect(x, y) ? inner : outer;
			image.SetRGB(x, y, colour.Red(), colour.Green(), colour.Blue());
			image.SetAlpha(x, y, 255);
		}
	}

	wxBitmap bitmap(image);
	if (bitmap.IsOk()) {
		return bitmap;
	}

	return wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_OTHER, wxSize(size, size));
}

// The application logo: prefer the shipped rpcemu.png, fall back to the
// procedural placeholder if it can't be found/loaded.
wxBitmap
AboutLogo(int size)
{
	const wxString path = wxString::FromUTF8(rpcemu_get_resourcedir()) +
	    "resources" + wxFileName::GetPathSeparator() + "rpcemu.png";
	wxImage image;

	if (wxFileExists(path) && image.LoadFile(path, wxBITMAP_TYPE_PNG)) {
		if (image.GetWidth() != size || image.GetHeight() != size) {
			image.Rescale(size, size, wxIMAGE_QUALITY_HIGH);
		}
		return wxBitmap(image);
	}

	return CreateAboutIcon(size);
}

wxString
BuildDescription()
{
#if defined(RPCEMU_BUILD_DYNAREC)
	const wxString core = "Dynamic recompiler";
#else
	const wxString core = "Interpreter";
#endif
	// wxString::Format format strings must be ASCII-only; UTF-8 breaks wxFormatString.
	return core + wxString::Format(" build, %ld-bit", static_cast<long>(sizeof(void *) * 8));
}

void
OpenUrl(const wxString &url)
{
	if (!url.empty()) {
		wxLaunchDefaultBrowser(url);
	}
}

} // namespace

enum {
	ID_ABOUT_MANUAL = wxID_HIGHEST + 510,
	ID_ABOUT_WEBSITE,
};

AboutDialog::AboutDialog(wxWindow *parent)
	: wxDialog(parent, wxID_ANY, "About RPCEmu", wxDefaultPosition, wxDefaultSize,
	           wxDEFAULT_DIALOG_STYLE | wxCLOSE_BOX)
{
	BuildUi();
	Fit();
	SetMinSize(GetSize());
	CentreOnParent();
}

void AboutDialog::BuildUi()
{
	const int year = wxDateTime::Now().GetYear();
	const wxColour muted = wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT);
	const wxColour link = wxSystemSettings::GetColour(wxSYS_COLOUR_HOTLIGHT);

	wxFont title_font = GetFont();
	title_font.SetPointSize(title_font.GetPointSize() + 6);
	title_font.SetWeight(wxFONTWEIGHT_BOLD);

	wxFont edition_font = GetFont();
	edition_font.SetPointSize(edition_font.GetPointSize() + 1);

	wxFont small_font = GetFont();
	small_font.SetPointSize(std::max(8, small_font.GetPointSize() - 1));

	auto *icon = new wxStaticBitmap(this, wxID_ANY, AboutLogo(64));

	auto *title = new wxStaticText(this, wxID_ANY, "RPCEmu");
	title->SetFont(title_font);

	auto *edition = new wxStaticText(this, wxID_ANY, "Spork Edition");
	edition->SetFont(edition_font);
	edition->SetForegroundColour(muted);

	auto *version = new wxStaticText(this, wxID_ANY, "Version " + wxString(VERSION));

	auto *title_block = new wxBoxSizer(wxVERTICAL);
	title_block->Add(title, 0, wxBOTTOM, 2);
	title_block->Add(edition, 0, wxBOTTOM, 4);
	title_block->Add(version, 0);

	auto *header = new wxBoxSizer(wxHORIZONTAL);
	header->Add(icon, 0, wxALIGN_CENTRE_VERTICAL | wxRIGHT, 16);
	header->Add(title_block, 1, wxALIGN_CENTRE_VERTICAL);

	auto *tagline = new wxStaticText(this, wxID_ANY, "Acorn Risc PC and A7000 emulator");

	const wxString copyright =
	    wxString::Format("Copyright 2005-%d RPCEmu contributors", year) +
	    "\nSpork Edition enhancements by Andrew Timmins";

	auto *copyright_label = new wxStaticText(this, wxID_ANY, copyright);
	copyright_label->Wrap(400);

	auto *license = new wxStaticText(this, wxID_ANY,
	                                 "This program is free software, licensed under the "
	                                 "GNU General Public License, version 2. "
	                                 "See the COPYING file for details.");
	license->Wrap(400);
	license->SetForegroundColour(muted);

	auto *build_info = new wxStaticText(this, wxID_ANY, BuildDescription());
	build_info->SetFont(small_font);
	build_info->SetForegroundColour(muted);

	auto *manual_link = new wxButton(this, ID_ABOUT_MANUAL, "Documentation",
	                                 wxDefaultPosition, wxDefaultSize,
	                                 wxBORDER_NONE | wxBU_LEFT);
	manual_link->SetForegroundColour(link);
	manual_link->SetCursor(wxCursor(wxCURSOR_HAND));

	auto *website_link = new wxButton(this, ID_ABOUT_WEBSITE, "GitHub repository",
	                                  wxDefaultPosition, wxDefaultSize,
	                                  wxBORDER_NONE | wxBU_LEFT);
	website_link->SetForegroundColour(link);
	website_link->SetCursor(wxCursor(wxCURSOR_HAND));

	manual_link->Bind(wxEVT_BUTTON, [](wxCommandEvent &) { OpenUrl(URL_MANUAL); });
	website_link->Bind(wxEVT_BUTTON, [](wxCommandEvent &) { OpenUrl(URL_WEBSITE); });

	auto *links = new wxBoxSizer(wxHORIZONTAL);
	links->Add(manual_link, 0, wxRIGHT, 16);
	links->Add(website_link, 0);

	auto *buttons = CreateStdDialogButtonSizer(wxOK);

	auto *main = new wxBoxSizer(wxVERTICAL);
	main->Add(header, 0, wxEXPAND | wxALL, 16);
	main->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, 16);
	main->Add(tagline, 0, wxALL, 16);
	main->Add(copyright_label, 0, wxLEFT | wxRIGHT | wxBOTTOM, 16);
	main->Add(license, 0, wxLEFT | wxRIGHT | wxBOTTOM, 16);
	main->Add(build_info, 0, wxLEFT | wxRIGHT | wxBOTTOM, 16);
	main->Add(links, 0, wxLEFT | wxRIGHT | wxBOTTOM, 16);
	main->Add(buttons, 0, wxEXPAND | wxALL, 10);
	SetSizer(main);
}
