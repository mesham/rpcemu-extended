/*
  RPCEmu - An Acorn system emulator

  Copyright (C) 2025-2026 Andy Timmins

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
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
