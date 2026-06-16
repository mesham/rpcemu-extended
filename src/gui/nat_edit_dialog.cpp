#include "nat_edit_dialog.h"

#include "nat_list_dialog.h"

enum {
	ID_NAT_EDIT_SAVE = wxID_HIGHEST + 200,
};

NatEditDialog::NatEditDialog(NatListDialog *parent)
	: wxDialog(parent, wxID_ANY, "Configure NAT Port Forwarding Rules", wxDefaultPosition, wxDefaultSize,
	           wxDEFAULT_DIALOG_STYLE | wxCLOSE_BOX)
	, nat_list_dialog_(parent)
{
	BuildUi();
	Fit();
	SetMinSize(GetSize());
	CentreOnParent();
}

void NatEditDialog::BuildUi()
{
	proto_combo_ = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr,
	                              wxCB_READONLY);
	proto_combo_->Append("TCP");
	proto_combo_->Append("UDP");

	emu_port_spin_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(100, -1),
	                                wxSP_ARROW_KEYS, 1, 65535, 1);
	host_port_spin_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(100, -1),
	                                 wxSP_ARROW_KEYS, 1, 65535, 1);

	auto *form = new wxFlexGridSizer(2, 8, 8);
	form->AddGrowableCol(1, 1);
	form->Add(new wxStaticText(this, wxID_ANY, "Protocol"), 0, wxALIGN_CENTER_VERTICAL);
	form->Add(proto_combo_, 1, wxEXPAND);
	form->Add(new wxStaticText(this, wxID_ANY, "Emulator Port"), 0, wxALIGN_CENTER_VERTICAL);
	form->Add(emu_port_spin_, 1, wxEXPAND);
	form->Add(new wxStaticText(this, wxID_ANY, "Host Port"), 0, wxALIGN_CENTER_VERTICAL);
	form->Add(host_port_spin_, 1, wxEXPAND);

	auto *save_btn = new wxButton(this, ID_NAT_EDIT_SAVE, "Save Changes");
	auto *cancel_btn = new wxButton(this, wxID_CANCEL, "Cancel");
	save_btn->SetDefault();

	auto *button_row = new wxBoxSizer(wxHORIZONTAL);
	button_row->AddStretchSpacer();
	button_row->Add(save_btn, 0, wxRIGHT, 4);
	button_row->Add(cancel_btn, 0);

	auto *main = new wxBoxSizer(wxVERTICAL);
	main->Add(form, 0, wxEXPAND | wxALL, 10);
	main->Add(button_row, 0, wxEXPAND | wxALL, 10);
	SetSizer(main);

	save_btn->Bind(wxEVT_BUTTON, &NatEditDialog::OnSave, this);
	cancel_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { EndModal(wxID_CANCEL); });
}

void NatEditDialog::SetValues(bool is_edit, PortForwardRule rule)
{
	is_edit_ = is_edit;
	old_rule_ = rule;

	if (is_edit_) {
		SetTitle("Edit Rule");
		proto_combo_->SetSelection(rule.type == PORT_FORWARD_TCP ? 0 : 1);
		emu_port_spin_->SetValue(rule.emu_port);
		host_port_spin_->SetValue(rule.host_port);
	} else {
		SetTitle("Add Rule");
		proto_combo_->SetSelection(0);
		emu_port_spin_->SetValue(1);
		host_port_spin_->SetValue(1);
	}
}

void NatEditDialog::OnSave(wxCommandEvent &)
{
	PortForwardRule new_rule{};
	new_rule.type = proto_combo_->GetSelection() == 0 ? PORT_FORWARD_TCP : PORT_FORWARD_UDP;
	new_rule.emu_port = static_cast<uint16_t>(emu_port_spin_->GetValue());
	new_rule.host_port = static_cast<uint16_t>(host_port_spin_->GetValue());

	if (is_edit_) {
		if (new_rule.type == old_rule_.type
		    && new_rule.emu_port == old_rule_.emu_port
		    && new_rule.host_port == old_rule_.host_port) {
			EndModal(wxID_OK);
			return;
		}

		if (!nat_list_dialog_->ValidateEdit(old_rule_, new_rule)) {
			return;
		}

		nat_list_dialog_->ProcessEdit(old_rule_, new_rule);
	} else {
		if (!nat_list_dialog_->ValidateAdd(new_rule)) {
			return;
		}

		nat_list_dialog_->ProcessAdd(new_rule);
	}

	EndModal(wxID_OK);
}
