/*
  RPCEmu - An Acorn system emulator

  Per-podule configuration dialog - see podule_config_dialog.h.
 */

#include "podule_config_dialog.h"

#include <wx/statline.h>

#include <cstdlib>

namespace {

/* Current stored value for an item, or the supplied default. */
wxString item_value(std::map<wxString, wxString> *values, const char *name, const wxString &def)
{
	if (name != nullptr) {
		auto it = values->find(wxString::FromUTF8(name));
		if (it != values->end()) {
			return it->second;
		}
	}
	return def;
}

} // namespace

PoduleConfigDialog::PoduleConfigDialog(wxWindow *parent, const wxString &title,
                                       const podule_config_t *config,
                                       std::map<wxString, wxString> *values)
    : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE),
      values_(values)
{
	auto *root = new wxBoxSizer(wxVERTICAL);
	auto *grid = new wxFlexGridSizer(0, 2, 6, 8);
	grid->AddGrowableCol(1, 1);

	for (const podule_config_item_t *item = config->items; item->type != -1; item++) {
		switch (item->type) {
		case CONFIG_STRING: {
			grid->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8(item->description)),
			          0, wxALIGN_CENTER_VERTICAL);
			const wxString cur = item_value(values_, item->name,
			    item->default_string ? wxString::FromUTF8(item->default_string) : wxString());
			auto *t = new wxTextCtrl(this, wxID_ANY, cur, wxDefaultPosition, wxSize(280, -1));
			grid->Add(t, 1, wxEXPAND);
			controls_.push_back({item, t});
			break;
		}
		case CONFIG_INT: {
			grid->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8(item->description)),
			          0, wxALIGN_CENTER_VERTICAL);
			const wxString cur = item_value(values_, item->name,
			    wxString::Format("%d", item->default_int));
			auto *t = new wxTextCtrl(this, wxID_ANY, cur);
			grid->Add(t, 1, wxEXPAND);
			controls_.push_back({item, t});
			break;
		}
		case CONFIG_BINARY: {
			auto *cb = new wxCheckBox(this, wxID_ANY, wxString::FromUTF8(item->description));
			const wxString cur = item_value(values_, item->name,
			    wxString::Format("%d", item->default_int));
			cb->SetValue(atoi(cur.utf8_str()) != 0);
			grid->Add(new wxStaticText(this, wxID_ANY, wxEmptyString), 0);
			grid->Add(cb, 1, wxEXPAND);
			controls_.push_back({item, cb});
			break;
		}
		case CONFIG_SELECTION: {
			grid->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8(item->description)),
			          0, wxALIGN_CENTER_VERTICAL);
			auto *ch = new wxChoice(this, wxID_ANY);
			const int cur_val = atoi(item_value(values_, item->name,
			    wxString::Format("%d", item->default_int)).utf8_str());
			int sel = 0, idx = 0;
			for (const podule_config_selection_t *s = item->selection;
			     s && s->description && s->description[0]; s++, idx++) {
				ch->Append(wxString::FromUTF8(s->description));
				if (s->value == cur_val) {
					sel = idx;
				}
			}
			ch->SetSelection(sel);
			grid->Add(ch, 1, wxEXPAND);
			controls_.push_back({item, ch});
			break;
		}
		case CONFIG_SELECTION_STRING: {
			grid->Add(new wxStaticText(this, wxID_ANY, wxString::FromUTF8(item->description)),
			          0, wxALIGN_CENTER_VERTICAL);
			auto *ch = new wxChoice(this, wxID_ANY);
			const wxString cur = item_value(values_, item->name,
			    item->default_string ? wxString::FromUTF8(item->default_string) : wxString());
			int sel = 0, idx = 0;
			for (const podule_config_selection_t *s = item->selection;
			     s && s->description && s->description[0]; s++, idx++) {
				ch->Append(wxString::FromUTF8(s->description));
				if (s->value_string && cur == wxString::FromUTF8(s->value_string)) {
					sel = idx;
				}
			}
			ch->SetSelection(sel);
			grid->Add(ch, 1, wxEXPAND);
			controls_.push_back({item, ch});
			break;
		}
		default:
			/* CONFIG_BUTTON (file pickers etc.) needs the GUI callback set,
			   which isn't wired yet - skip rather than render dead controls. */
			break;
		}
	}

	root->Add(grid, 1, wxEXPAND | wxALL, 12);
	root->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
	if (wxSizer *buttons = CreateButtonSizer(wxOK | wxCANCEL)) {
		root->Add(buttons, 0, wxEXPAND | wxALL, 8);
	}

	Bind(wxEVT_BUTTON, &PoduleConfigDialog::OnOk, this, wxID_OK);
	SetSizerAndFit(root);
	CentreOnParent();
}

void PoduleConfigDialog::OnOk(wxCommandEvent &event)
{
	for (const auto &ic : controls_) {
		if (ic.item->name == nullptr) {
			continue;
		}
		const wxString key = wxString::FromUTF8(ic.item->name);

		switch (ic.item->type) {
		case CONFIG_STRING:
		case CONFIG_INT:
			(*values_)[key] = static_cast<wxTextCtrl *>(ic.ctrl)->GetValue();
			break;
		case CONFIG_BINARY:
			(*values_)[key] = static_cast<wxCheckBox *>(ic.ctrl)->GetValue() ? "1" : "0";
			break;
		case CONFIG_SELECTION: {
			const int sel = static_cast<wxChoice *>(ic.ctrl)->GetSelection();
			int idx = 0, val = ic.item->default_int;
			for (const podule_config_selection_t *s = ic.item->selection;
			     s && s->description && s->description[0]; s++, idx++) {
				if (idx == sel) {
					val = s->value;
					break;
				}
			}
			(*values_)[key] = wxString::Format("%d", val);
			break;
		}
		case CONFIG_SELECTION_STRING: {
			const int sel = static_cast<wxChoice *>(ic.ctrl)->GetSelection();
			int idx = 0;
			for (const podule_config_selection_t *s = ic.item->selection;
			     s && s->description && s->description[0]; s++, idx++) {
				if (idx == sel && s->value_string) {
					(*values_)[key] = wxString::FromUTF8(s->value_string);
					break;
				}
			}
			break;
		}
		default:
			break;
		}
	}
	event.Skip();
}
