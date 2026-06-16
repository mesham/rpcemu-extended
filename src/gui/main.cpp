#include <wx/wx.h>

#include "data_paths.h"
#include "config_paths.h"
#include "config_selector_dialog.h"
#include "main_frame.h"

extern "C" {
#include "rpcemu.h"
}

class RpcemuApp : public wxApp {
public:
	bool OnInit() override;
};

wxIMPLEMENT_APP(RpcemuApp);

bool RpcemuApp::OnInit()
{
	if (!wxApp::OnInit()) {
		return false;
	}

	InitRpcemuPaths();

	ConfigSelectorDialog selector(nullptr);
	if (selector.ShowModal() != wxID_OK) {
		return false;
	}

	const wxString config_path = selector.GetSelectedConfigPath();
	config_set_path(ConfigPathsAbsoluteConfigPath(config_path).utf8_str().data());
	rpcemu_prestart();

	auto *frame = new MainFrame();
	frame->Show(true);
	SetTopWindow(frame);

	rpcemu_start();
	frame->StartEmulator();
	return true;
}
