#include "nat_list_dialog.h"

#include "nat_edit_dialog.h"

enum {
	ID_NAT_LIST_ADD = wxID_HIGHEST + 210,
	ID_NAT_LIST_EDIT,
	ID_NAT_LIST_DELETE,
};

static NatListDialog *g_active_nat_list_dialog = nullptr;

NatListDialog::NatListDialog(wxWindow *parent, EmulatorHost *emulator_host)
	: wxDialog(parent, wxID_ANY, "Configure NAT Port Forwarding Rules", wxDefaultPosition, wxSize(560, 360),
	           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxCLOSE_BOX)
	, emulator_host_(emulator_host)
{
	BuildUi();
	LoadRulesFromConfig();
	Fit();
	CentreOnParent();

	g_active_nat_list_dialog = this;
}

NatListDialog::~NatListDialog()
{
	if (g_active_nat_list_dialog == this) {
		g_active_nat_list_dialog = nullptr;
	}
	delete nat_edit_dialog_;
}

void NatListDialogNotifyRule(PortForwardRule rule)
{
	if (g_active_nat_list_dialog != nullptr) {
		g_active_nat_list_dialog->AddNatRule(rule);
	}
}

void NatListDialog::BuildUi()
{
	rules_list_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
	                             wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SUNKEN);
	rules_list_->AppendColumn("Protocol", wxLIST_FORMAT_LEFT, 90);
	rules_list_->AppendColumn("Emulator Port", wxLIST_FORMAT_LEFT, 120);
	rules_list_->AppendColumn("Host Port", wxLIST_FORMAT_LEFT, 120);

	nat_edit_dialog_ = new NatEditDialog(this);

	auto *add_btn = new wxButton(this, ID_NAT_LIST_ADD, "Add Rule");
	auto *edit_btn = new wxButton(this, ID_NAT_LIST_EDIT, "Edit");
	auto *delete_btn = new wxButton(this, ID_NAT_LIST_DELETE, "Delete");
	auto *close_btn = new wxButton(this, wxID_CLOSE, "Close");
	add_btn->SetDefault();

	auto *button_row = new wxBoxSizer(wxHORIZONTAL);
	button_row->Add(add_btn, 0, wxRIGHT, 4);
	button_row->Add(edit_btn, 0, wxRIGHT, 4);
	button_row->Add(delete_btn, 0, wxRIGHT, 4);
	button_row->AddStretchSpacer();
	button_row->Add(close_btn, 0);

	auto *main = new wxBoxSizer(wxVERTICAL);
	main->Add(rules_list_, 1, wxEXPAND | wxALL, 10);
	main->Add(button_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
	SetSizer(main);

	add_btn->Bind(wxEVT_BUTTON, &NatListDialog::OnAddRule, this);
	edit_btn->Bind(wxEVT_BUTTON, &NatListDialog::OnEditRule, this);
	delete_btn->Bind(wxEVT_BUTTON, &NatListDialog::OnDeleteRule, this);
	close_btn->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { EndModal(wxID_CLOSE); });
	rules_list_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &NatListDialog::OnListActivated, this);
}

void NatListDialog::LoadRulesFromConfig()
{
	rules_list_->DeleteAllItems();
	for (int i = 0; i < MAX_PORT_FORWARDS; ++i) {
		if (port_forward_rules[i].type != PORT_FORWARD_NONE) {
			AddNatRule(port_forward_rules[i]);
		}
	}
}

PortForwardRule NatListDialog::RuleFromRow(long row) const
{
	PortForwardRule rule{};
	const wxString proto = rules_list_->GetItemText(row, 0);
	rule.type = proto == "TCP" ? PORT_FORWARD_TCP : PORT_FORWARD_UDP;

	unsigned long emu = 0;
	unsigned long host = 0;
	rules_list_->GetItemText(row, 1).ToULong(&emu);
	rules_list_->GetItemText(row, 2).ToULong(&host);
	rule.emu_port = static_cast<uint16_t>(emu);
	rule.host_port = static_cast<uint16_t>(host);
	return rule;
}

long NatListDialog::SelectedRow() const
{
	long item = -1;
	item = rules_list_->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
	return item;
}

void NatListDialog::OnAddRule(wxCommandEvent &)
{
	if (rules_list_->GetItemCount() >= MAX_PORT_FORWARDS) {
		error("You are at the maximum number of rules. Please remove a rule to make space for a new one.");
		return;
	}

	const PortForwardRule rule{PORT_FORWARD_TCP, 1, 1};
	nat_edit_dialog_->SetValues(false, rule);
	nat_edit_dialog_->ShowModal();
}

void NatListDialog::OnEditRule(wxCommandEvent &)
{
	const long row = SelectedRow();
	if (row == wxNOT_FOUND) {
		return;
	}

	const PortForwardRule rule = RuleFromRow(row);
	nat_edit_dialog_->SetValues(true, rule);
	nat_edit_dialog_->ShowModal();
}

void NatListDialog::OnDeleteRule(wxCommandEvent &)
{
	const long row = SelectedRow();
	if (row == wxNOT_FOUND) {
		return;
	}

	const PortForwardRule rule = RuleFromRow(row);
	const wxString message = wxString::Format(
	    "Are you sure you want to delete NAT rule %s, emu port %u, host port %u?",
	    rule.type == PORT_FORWARD_TCP ? "TCP" : "UDP",
	    rule.emu_port,
	    rule.host_port);

	if (wxMessageBox(message, "RPCEmu", wxYES_NO | wxICON_QUESTION | wxNO_DEFAULT, this) != wxYES) {
		return;
	}

	rules_list_->DeleteItem(row);

	if (emulator_host_ != nullptr) {
		emulator_host_->NatRuleRemove(rule);
	}
}

void NatListDialog::OnListActivated(wxListEvent &)
{
	wxCommandEvent dummy;
	OnEditRule(dummy);
}

void NatListDialog::AddNatRule(PortForwardRule rule)
{
	long insert_row = 0;
	const long count = rules_list_->GetItemCount();
	while (insert_row < count) {
		unsigned long row_emu = 0;
		rules_list_->GetItemText(insert_row, 1).ToULong(&row_emu);

		if (rule.emu_port < static_cast<uint16_t>(row_emu)) {
			break;
		}
		if (rule.emu_port == static_cast<uint16_t>(row_emu)) {
			if (rule.type == PORT_FORWARD_TCP) {
				break;
			}
			++insert_row;
			break;
		}
		++insert_row;
	}

	rules_list_->InsertItem(insert_row, rule.type == PORT_FORWARD_TCP ? "TCP" : "UDP");
	rules_list_->SetItem(insert_row, 1, wxString::Format("%u", rule.emu_port));
	rules_list_->SetItem(insert_row, 2, wxString::Format("%u", rule.host_port));
}

bool NatListDialog::ValidateAdd(PortForwardRule rule)
{
	const long count = rules_list_->GetItemCount();
	for (long i = 0; i < count; ++i) {
		const PortForwardRule row_rule = RuleFromRow(i);

		if (rule.type == row_rule.type && rule.emu_port == row_rule.emu_port) {
			error("Emulated port is in use in another rule");
			return false;
		}

		if (rule.type == row_rule.type && rule.host_port == row_rule.host_port) {
			error("Host port is in use in another rule");
			return false;
		}
	}

	return true;
}

bool NatListDialog::ValidateEdit(PortForwardRule old_rule, PortForwardRule new_rule)
{
	const long count = rules_list_->GetItemCount();
	for (long i = 0; i < count; ++i) {
		const PortForwardRule row_rule = RuleFromRow(i);

		if (row_rule.type == old_rule.type
		    && row_rule.emu_port == old_rule.emu_port
		    && row_rule.host_port == old_rule.host_port) {
			continue;
		}

		if (new_rule.type == row_rule.type && new_rule.emu_port == row_rule.emu_port) {
			error("Emulated port is in use in another rule");
			return false;
		}

		if (new_rule.type == row_rule.type && new_rule.host_port == row_rule.host_port) {
			error("Host port is in use in another rule");
			return false;
		}
	}

	return true;
}

void NatListDialog::ProcessAdd(PortForwardRule rule)
{
	AddNatRule(rule);

	if (emulator_host_ != nullptr) {
		emulator_host_->NatRuleAdd(rule);
	}
}

void NatListDialog::ProcessEdit(PortForwardRule old_rule, PortForwardRule new_rule)
{
	const long count = rules_list_->GetItemCount();
	for (long i = 0; i < count; ++i) {
		const PortForwardRule row_rule = RuleFromRow(i);
		if (row_rule.type == old_rule.type
		    && row_rule.emu_port == old_rule.emu_port
		    && row_rule.host_port == old_rule.host_port) {
			rules_list_->DeleteItem(i);
			break;
		}
	}

	AddNatRule(new_rule);

	if (emulator_host_ != nullptr) {
		emulator_host_->NatRuleEdit(old_rule, new_rule);
	}
}
