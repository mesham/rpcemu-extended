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
