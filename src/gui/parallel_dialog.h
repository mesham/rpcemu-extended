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

#ifndef PARALLEL_DIALOG_H
#define PARALLEL_DIALOG_H

#include <wx/wx.h>

enum class ParallelPortMode {
	Disabled,
	LogToFile,
	VirtualPrinter,
	PhysicalDevice
};

struct ParallelPortSettings {
	ParallelPortMode mode = ParallelPortMode::Disabled;
	wxString log_file_path;
	wxString printer_output_folder;
	wxString physical_device;
	bool printer_auto_pdf = false;
};

class ParallelDialog : public wxDialog {
public:
	explicit ParallelDialog(wxWindow *parent);

	ParallelPortSettings GetSettings() const;
	void SetSettings(const ParallelPortSettings &settings);

private:
	void BuildUi();
	void UpdateModeWidgets();
	bool ApplySettings();

	void OnModeChanged(wxCommandEvent &event);
	void OnBrowseLogFile(wxCommandEvent &event);
	void OnBrowsePrinterFolder(wxCommandEvent &event);
	void OnApply(wxCommandEvent &event);

	wxRadioButton *disabled_radio_ = nullptr;
	wxRadioButton *logfile_radio_ = nullptr;
	wxRadioButton *printer_radio_ = nullptr;
	wxRadioButton *physical_radio_ = nullptr;
	wxTextCtrl *logfile_edit_ = nullptr;
	wxButton *logbrowse_btn_ = nullptr;
	wxTextCtrl *printer_output_edit_ = nullptr;
	wxButton *printer_browse_btn_ = nullptr;
	wxStaticText *printer_help_ = nullptr;
	wxCheckBox *printer_auto_pdf_checkbox_ = nullptr;
	wxComboBox *device_combo_ = nullptr;
};

#endif
