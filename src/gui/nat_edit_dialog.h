#ifndef NAT_EDIT_DIALOG_H
#define NAT_EDIT_DIALOG_H

#include <wx/wx.h>
#include <wx/spinctrl.h>

extern "C" {
#include "rpcemu.h"
}

class NatListDialog;

class NatEditDialog : public wxDialog {
public:
	NatEditDialog(NatListDialog *parent);

	void SetValues(bool is_edit, PortForwardRule rule);

private:
	void BuildUi();
	void OnSave(wxCommandEvent &event);

	NatListDialog *nat_list_dialog_ = nullptr;
	wxComboBox *proto_combo_ = nullptr;
	wxSpinCtrl *emu_port_spin_ = nullptr;
	wxSpinCtrl *host_port_spin_ = nullptr;

	bool is_edit_ = false;
	PortForwardRule old_rule_{};
};

#endif
