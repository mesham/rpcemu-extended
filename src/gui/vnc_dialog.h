#ifdef RPCEMU_VNC

#ifndef VNC_DIALOG_H
#define VNC_DIALOG_H

#include <wx/wx.h>
#include <wx/spinctrl.h>

#include "vnc_server.h"

extern "C" {
#include "rpcemu.h"
}

class VncDialog : public wxDialog {
public:
	VncDialog(wxWindow *parent, VncServer *server, const wxString &current_password, Config *config_copy);

	wxString GetPassword() const { return password_edit_->GetValue(); }

private:
	void BuildUi();
	void UpdateStatus();
	bool ApplySettings();
	void OnEnable(wxCommandEvent &event);
	void OnApply(wxCommandEvent &event);
	void OnOk(wxCommandEvent &event);

	VncServer *vnc_server_;
	Config *config_copy_;
	wxCheckBox *enable_checkbox_ = nullptr;
	wxSpinCtrl *port_spin_ = nullptr;
	wxTextCtrl *password_edit_ = nullptr;
	wxStaticText *status_label_ = nullptr;
	wxStaticText *clients_label_ = nullptr;
};

#endif /* VNC_DIALOG_H */

#endif /* RPCEMU_VNC */
