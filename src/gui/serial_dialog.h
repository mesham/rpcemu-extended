#ifndef SERIAL_DIALOG_H
#define SERIAL_DIALOG_H

#include <wx/wx.h>

enum class SerialPortMode {
	Disabled,
	LogToFile,
	TcpModem,
	PhysicalDevice
};

struct SerialPortSettings {
	SerialPortMode mode = SerialPortMode::Disabled;
	wxString log_file_path;
	wxString physical_device;
};

class SerialDialog : public wxDialog {
public:
	explicit SerialDialog(wxWindow *parent);

	SerialPortSettings GetCom1Settings() const;
	void SetCom1Settings(const SerialPortSettings &settings);

private:
	struct PortWidgets {
		wxRadioButton *disabled_radio = nullptr;
		wxRadioButton *logfile_radio = nullptr;
		wxRadioButton *tcpmodem_radio = nullptr;
		wxRadioButton *physical_radio = nullptr;
		wxTextCtrl *logfile_edit = nullptr;
		wxButton *browse_btn = nullptr;
		wxComboBox *device_combo = nullptr;
	};

	void BuildUi();
	wxStaticBoxSizer *CreatePortGroup(const wxString &title);
	void UpdatePortWidgets(PortWidgets &widgets);
	bool ApplySettings();

	void OnCom1ModeChanged(wxCommandEvent &event);
	void OnBrowseLogFile1(wxCommandEvent &event);
	void OnApply(wxCommandEvent &event);

	PortWidgets com1_;
};

#endif
