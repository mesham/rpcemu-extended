#ifndef NAT_LIST_DIALOG_H
#define NAT_LIST_DIALOG_H

#include <wx/wx.h>
#include <wx/listctrl.h>

#include "emulator_host.h"

extern "C" {
#include "rpcemu.h"
}

class NatEditDialog;

class NatListDialog : public wxDialog {
public:
	NatListDialog(wxWindow *parent, EmulatorHost *emulator_host);
	~NatListDialog() override;

	void AddNatRule(PortForwardRule rule);

	bool ValidateAdd(PortForwardRule rule);
	bool ValidateEdit(PortForwardRule old_rule, PortForwardRule new_rule);
	void ProcessAdd(PortForwardRule rule);
	void ProcessEdit(PortForwardRule old_rule, PortForwardRule new_rule);

private:
	void BuildUi();
	void LoadRulesFromConfig();
	PortForwardRule RuleFromRow(long row) const;
	long SelectedRow() const;

	void OnAddRule(wxCommandEvent &event);
	void OnEditRule(wxCommandEvent &event);
	void OnDeleteRule(wxCommandEvent &event);
	void OnListActivated(wxListEvent &event);

	EmulatorHost *emulator_host_ = nullptr;
	wxListCtrl *rules_list_ = nullptr;
	NatEditDialog *nat_edit_dialog_ = nullptr;
};

void NatListDialogNotifyRule(PortForwardRule rule);

#endif
