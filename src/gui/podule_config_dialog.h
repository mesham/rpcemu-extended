/*
  RPCEmu - An Acorn system emulator

  Per-podule configuration dialog. Renders a podule's self-describing
  podule_config_t schema (text / checkbox / selection items) into wx controls
  and reads/writes the values into a per-section key/value map owned by the
  caller (the machine-edit dialog), so the configuration stays per-machine.
 */

#ifndef PODULE_CONFIG_DIALOG_H
#define PODULE_CONFIG_DIALOG_H

#include <wx/wx.h>
#include <wx/dialog.h>

#include <map>
#include <vector>

extern "C" {
#include "podule_api.h"
}

class PoduleConfigDialog : public wxDialog {
public:
	PoduleConfigDialog(wxWindow *parent, const wxString &title,
	                   const podule_config_t *config,
	                   std::map<wxString, wxString> *values);

private:
	void OnOk(wxCommandEvent &event);

	struct ItemControl {
		const podule_config_item_t *item;
		wxControl *ctrl;
	};

	std::map<wxString, wxString> *values_;
	std::vector<ItemControl> controls_;
};

#endif /* PODULE_CONFIG_DIALOG_H */
