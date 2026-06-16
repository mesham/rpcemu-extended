#ifndef MACHINE_EDIT_DIALOG_H
#define MACHINE_EDIT_DIALOG_H

#include <wx/wx.h>

extern "C" {
#include "rpcemu.h"
}

class MachineEditDialog : public wxDialog {
public:
	MachineEditDialog(wxWindow *parent, const wxString &config_path, bool allow_rename = true,
	                  bool emulator_running = false);

	wxString GetNewName() const { return new_name_; }
	bool WasRenamed() const { return renamed_; }

private:
	enum class HardDiscState {
		Missing,
		Empty,
		Ready,
		Blocked,
		CustomPath,
	};

	struct HardDiscInfo {
		HardDiscState state = HardDiscState::Missing;
		wxString path;
		wxString size_text;
		wxString modified_text;
		bool uses_custom_path = false;
	};

	struct HardDiscPanel {
		wxStaticText *badge = nullptr;
		wxStaticText *path_label = nullptr;
		wxStaticText *modified_label = nullptr;
		wxButton *create_btn = nullptr;
		wxButton *delete_btn = nullptr;
		wxButton *open_folder_btn = nullptr;
		int drive_num = 0;
	};

	void BuildUi();
	void BuildHardDiscPanel(wxWindow *parent, wxSizer *parent_sizer, HardDiscPanel &panel, int drive_num,
	                        int ide_index);
	void LoadSettings();
	void SaveSettings();
	void ApplySavedSettingsToGlobalConfig(const wxString &rom_dir, int mem_size, int vram_internal,
	                                      int refresh, NetworkType network_type);
	void PopulateRomList();
	void UpdateRomModelCompatibility();
	void UpdateHdStatus();
	void ApplyHardDiscPanel(HardDiscPanel &panel, const HardDiscInfo &info);
	HardDiscInfo QueryHardDiscInfo(int drive) const;
	wxString CurrentMachineNameForHd() const;
	wxString HardDiscFilePath(int drive) const;
	void CreateHardDisc(int drive, int size_mb);
	void DeleteHardDisc(int drive);
	void OpenHardDiscFolder(int drive);
	void ShowHardDiscCreateMenu(int drive);
	Model CurrentModelSelection() const;

	void OnOk(wxCommandEvent &event);
	void OnNetworkChanged(wxCommandEvent &event);
	void OnRomOrModelChanged(wxCommandEvent &event);
	void OnNameChanged(wxCommandEvent &event);
	wxString SelectedRomDir() const;
	void SetRomSelection(const wxString &rom_dir);

	wxString config_path_;
	wxString original_name_;
	wxString new_name_;
	wxString hd4_path_;
	bool renamed_ = false;
	bool allow_rename_ = true;
	bool loading_settings_ = false;
	bool emulator_running_ = false;
	bool cdrom_enabled_ = false;

	wxTextCtrl *name_edit_ = nullptr;
	wxComboBox *rom_combo_ = nullptr;
	wxComboBox *model_combo_ = nullptr;
	wxComboBox *mem_combo_ = nullptr;
	wxComboBox *vram_combo_ = nullptr;
	wxSlider *refresh_slider_ = nullptr;
	wxStaticText *refresh_label_ = nullptr;
	wxComboBox *network_combo_ = nullptr;
	wxTextCtrl *bridge_edit_ = nullptr;
	wxStaticText *bridge_label_ = nullptr;
	wxTextCtrl *tunnel_edit_ = nullptr;
	wxStaticText *tunnel_label_ = nullptr;
	wxStaticText *compat_label_ = nullptr;
	wxStaticText *hd_reset_note_ = nullptr;
	HardDiscPanel hd4_panel_;
	HardDiscPanel hd5_panel_;
};

#endif
