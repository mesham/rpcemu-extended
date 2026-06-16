#ifndef CONFIG_SELECTOR_DIALOG_H
#define CONFIG_SELECTOR_DIALOG_H

#include <vector>

#include <wx/wx.h>

struct ConfigEntry {
	wxString display_name;
	wxString config_path;
};

class ConfigSelectorDialog : public wxDialog {
public:
	ConfigSelectorDialog(wxWindow *parent);

	wxString GetSelectedConfigPath() const { return selected_config_path_; }

private:
	void EnsureDefaultConfig();
	void RefreshConfigList();
	void UpdateButtons();
	wxString SelectedConfigPath() const;

	void OnStart(wxCommandEvent &event);
	void OnCancel(wxCommandEvent &event);
	void OnListDoubleClick(wxCommandEvent &event);
	void OnNew(wxCommandEvent &event);
	void OnEdit(wxCommandEvent &event);
	void OnDelete(wxCommandEvent &event);
	void OnClone(wxCommandEvent &event);

	wxListBox *config_list_ = nullptr;
	wxButton *new_button_ = nullptr;
	wxButton *edit_button_ = nullptr;
	wxButton *clone_button_ = nullptr;
	wxButton *delete_button_ = nullptr;
	wxButton *start_button_ = nullptr;
	wxString selected_config_path_;
	std::vector<ConfigEntry> config_entries_;
};

#endif
