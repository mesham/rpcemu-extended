#include "machine_edit_dialog.h"

#include "config_paths.h"

#include <cstring>

#include <wx/dir.h>
#include <wx/fileconf.h>
#include <wx/filename.h>
#include <wx/utils.h>

extern "C" {
#include "romload.h"
#include "rpcemu.h"
}

namespace {

const wxColour kHdColourMissing(120, 120, 120);
const wxColour kHdColourEmpty(180, 120, 0);
const wxColour kHdColourReady(27, 94, 32);
const wxColour kHdColourBlocked(120, 120, 120);
const wxColour kHdColourMuted(100, 100, 100);

wxString FormatHardDiscSize(wxULongLong size_bytes)
{
	if (size_bytes == 0) {
		return "0 bytes";
	}

	const double mb = size_bytes.ToDouble() / (1024.0 * 1024.0);
	if (mb < 1024.0) {
		return wxString::Format("%.1f MB", mb);
	}
	return wxString::Format("%.2f GB", mb / 1024.0);
}

wxString TruncatePathMiddle(const wxString &path, size_t max_len = 58)
{
	if (path.length() <= max_len) {
		return path;
	}

	const size_t keep = max_len - 3;
	const size_t front = keep / 2;
	const size_t back = keep - front;
	return path.substr(0, front) + "..." + path.substr(path.length() - back);
}

wxString FormatModifiedTime(const wxString &path)
{
	wxFileName file(path);
	if (!file.FileExists()) {
		return wxEmptyString;
	}

	const wxDateTime modified = file.GetModificationTime();
	if (!modified.IsValid()) {
		return wxEmptyString;
	}

	return modified.Format("Modified: %d %b %Y, %H:%M");
}

} // namespace

enum {
	ID_MACHINE_EDIT_OK = wxID_HIGHEST + 100,
	ID_HD_CREATE_256_MB,
	ID_HD_CREATE_512_MB,
	ID_HD_CREATE_1_GB,
	ID_HD_CREATE_2_GB,
};

MachineEditDialog::MachineEditDialog(wxWindow *parent, const wxString &config_path, bool allow_rename,
                                     bool emulator_running)
	: wxDialog(parent, wxID_ANY, "Edit Machine", wxDefaultPosition, wxSize(560, 620),
	           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
	, config_path_(config_path)
	, allow_rename_(allow_rename)
	, emulator_running_(emulator_running)
{
	BuildUi();
	LoadSettings();
	UpdateHdStatus();
	Fit();
	CentreOnParent();
}

void MachineEditDialog::BuildHardDiscPanel(wxWindow *parent, wxSizer *parent_sizer, HardDiscPanel &panel,
                                           int drive_num, int ide_index)
{
	panel.drive_num = drive_num;

	auto *drive_panel = new wxPanel(parent);
	auto *card = new wxBoxSizer(wxVERTICAL);

	card->Add(new wxStaticText(drive_panel, wxID_ANY,
	                           wxString::Format("HardDisc %d (IDE drive %d)", drive_num, ide_index)),
	          0, wxBOTTOM, 4);

	panel.badge = new wxStaticText(drive_panel, wxID_ANY, "Not created");
	panel.badge->SetFont(panel.badge->GetFont().Bold());
	card->Add(panel.badge, 0, wxEXPAND | wxBOTTOM, 4);

	panel.path_label = new wxStaticText(drive_panel, wxID_ANY, wxEmptyString);
	panel.path_label->SetForegroundColour(kHdColourMuted);
	card->Add(panel.path_label, 0, wxEXPAND | wxBOTTOM, 2);

	panel.modified_label = new wxStaticText(drive_panel, wxID_ANY, wxEmptyString);
	panel.modified_label->SetForegroundColour(kHdColourMuted);
	panel.modified_label->SetFont(panel.modified_label->GetFont().Smaller());
	card->Add(panel.modified_label, 0, wxEXPAND | wxBOTTOM, 8);

	auto *actions = new wxBoxSizer(wxHORIZONTAL);
	panel.create_btn = new wxButton(drive_panel, wxID_ANY, "New disc...");
	panel.delete_btn = new wxButton(drive_panel, wxID_ANY, "Delete");
	panel.open_folder_btn = new wxButton(drive_panel, wxID_ANY, "Open folder");
	actions->Add(panel.create_btn, 0, wxRIGHT, 4);
	actions->Add(panel.delete_btn, 0, wxRIGHT, 4);
	actions->Add(panel.open_folder_btn, 0);
	card->Add(actions, 0, wxEXPAND);

	drive_panel->SetSizer(card);
	parent_sizer->Add(drive_panel, 0, wxEXPAND | wxBOTTOM, 8);

	panel.create_btn->Bind(wxEVT_BUTTON, [this, drive_num](wxCommandEvent &) { ShowHardDiscCreateMenu(drive_num); });
	panel.delete_btn->Bind(wxEVT_BUTTON, [this, drive_num](wxCommandEvent &) { DeleteHardDisc(drive_num); });
	panel.open_folder_btn->Bind(wxEVT_BUTTON, [this, drive_num](wxCommandEvent &) { OpenHardDiscFolder(drive_num); });
}

void MachineEditDialog::BuildUi()
{
	name_edit_ = new wxTextCtrl(this, wxID_ANY);
	rom_combo_ = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
	model_combo_ = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
	mem_combo_ = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
	vram_combo_ = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
	refresh_slider_ = new wxSlider(this, wxID_ANY, 60, 20, 100);
	refresh_label_ = new wxStaticText(this, wxID_ANY, "60 Hz");
	network_combo_ = new wxComboBox(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY);
	bridge_label_ = new wxStaticText(this, wxID_ANY, "Bridge Name:");
	bridge_edit_ = new wxTextCtrl(this, wxID_ANY, "rpcemu");
	tunnel_label_ = new wxStaticText(this, wxID_ANY, "IP Address:");
	tunnel_edit_ = new wxTextCtrl(this, wxID_ANY, "172.31.0.1");
	compat_label_ = new wxStaticText(this, wxID_ANY, wxEmptyString);
	compat_label_->SetMinSize(wxSize(460, -1));

	PopulateRomList();

	for (int i = 0; i < Model_MAX; ++i) {
		model_combo_->Append(wxString::FromUTF8(models[i].name_gui));
	}

	const int mem_values[] = {4, 8, 16, 32, 64, 128, 256};
	for (int mem : mem_values) {
		mem_combo_->Append(wxString::Format("%d MB", mem));
	}
	vram_combo_->Append("None");
	vram_combo_->Append("2 MB");

	network_combo_->Append("Off");
	network_combo_->Append("NAT");
	network_combo_->Append("Ethernet Bridging");
	network_combo_->Append("IP Tunnelling");

	auto *form = new wxFlexGridSizer(2, 8, 8);
	form->AddGrowableCol(1, 1);
	form->Add(new wxStaticText(this, wxID_ANY, "Name:"), 0, wxALIGN_CENTER_VERTICAL);
	form->Add(name_edit_, 1, wxEXPAND);
	form->Add(new wxStaticText(this, wxID_ANY, "ROM:"), 0, wxALIGN_CENTER_VERTICAL);
	form->Add(rom_combo_, 1, wxEXPAND);
	form->Add(new wxStaticText(this, wxID_ANY, "Model:"), 0, wxALIGN_CENTER_VERTICAL);
	form->Add(model_combo_, 1, wxEXPAND);
	form->Add(new wxStaticText(this, wxID_ANY, "RAM:"), 0, wxALIGN_CENTER_VERTICAL);
	form->Add(mem_combo_, 1, wxEXPAND);
	form->Add(new wxStaticText(this, wxID_ANY, "VRAM:"), 0, wxALIGN_CENTER_VERTICAL);
	form->Add(vram_combo_, 1, wxEXPAND);
	form->Add(new wxStaticText(this, wxID_ANY, ""), 0);
	form->Add(compat_label_, 1, wxEXPAND);
	auto *refresh_row = new wxBoxSizer(wxHORIZONTAL);
	refresh_row->Add(refresh_slider_, 1, wxEXPAND | wxRIGHT, 8);
	refresh_row->Add(refresh_label_, 0, wxALIGN_CENTER_VERTICAL);
	form->Add(new wxStaticText(this, wxID_ANY, "Refresh Rate:"), 0, wxALIGN_CENTER_VERTICAL);
	form->Add(refresh_row, 1, wxEXPAND);
	form->Add(new wxStaticText(this, wxID_ANY, "Network:"), 0, wxALIGN_CENTER_VERTICAL);
	form->Add(network_combo_, 1, wxEXPAND);
	form->Add(bridge_label_, 0, wxALIGN_CENTER_VERTICAL);
	form->Add(bridge_edit_, 1, wxEXPAND);
	form->Add(tunnel_label_, 0, wxALIGN_CENTER_VERTICAL);
	form->Add(tunnel_edit_, 1, wxEXPAND);

	auto *hd_box = new wxStaticBoxSizer(wxVERTICAL, this, "Hard Discs");
	wxWindow *const hd_parent = hd_box->GetStaticBox();
	BuildHardDiscPanel(hd_parent, hd_box, hd4_panel_, 4, 0);
	BuildHardDiscPanel(hd_parent, hd_box, hd5_panel_, 5, 1);

	hd_reset_note_ = new wxStaticText(hd_parent, wxID_ANY,
	                                  "Changes to hard discs take effect after emulator reset.");
	hd_reset_note_->SetForegroundColour(kHdColourMuted);
	hd_reset_note_->SetFont(hd_reset_note_->GetFont().Smaller());
	hd_box->Add(hd_reset_note_, 0, wxEXPAND | wxTOP, 6);

	auto *button_row = new wxBoxSizer(wxHORIZONTAL);
	button_row->AddStretchSpacer();
	auto *ok_button = new wxButton(this, ID_MACHINE_EDIT_OK, "OK");
	auto *cancel_button = new wxButton(this, wxID_CANCEL, "Cancel");
	ok_button->SetDefault();
	button_row->Add(ok_button, 0, wxRIGHT, 4);
	button_row->Add(cancel_button, 0);

	auto *main = new wxBoxSizer(wxVERTICAL);
	main->Add(form, 0, wxEXPAND | wxALL, 10);
	main->Add(hd_box, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
	main->Add(button_row, 0, wxEXPAND | wxALL, 10);
	SetSizer(main);

	refresh_slider_->Bind(wxEVT_SLIDER, [this](wxCommandEvent &) {
		refresh_label_->SetLabel(wxString::Format("%d Hz", refresh_slider_->GetValue()));
	});
	network_combo_->Bind(wxEVT_COMBOBOX, &MachineEditDialog::OnNetworkChanged, this);
	rom_combo_->Bind(wxEVT_COMBOBOX, &MachineEditDialog::OnRomOrModelChanged, this);
	model_combo_->Bind(wxEVT_COMBOBOX, &MachineEditDialog::OnRomOrModelChanged, this);
	name_edit_->Bind(wxEVT_TEXT, &MachineEditDialog::OnNameChanged, this);
	ok_button->Bind(wxEVT_BUTTON, &MachineEditDialog::OnOk, this);
	cancel_button->Bind(wxEVT_BUTTON, [this](wxCommandEvent &) { EndModal(wxID_CANCEL); });

	hd_reset_note_->Show(emulator_running_);
}

void MachineEditDialog::PopulateRomList()
{
	rom_combo_->Clear();
	const wxString roms_dir = ConfigPathsRomsDir();
	if (!wxDirExists(roms_dir)) {
		rom_combo_->Append("(No roms/ directory found)");
		return;
	}

	rom_combo_->Append("(All files in roms/)", new wxStringClientData(wxEmptyString));

	wxDir dir(roms_dir);
	wxString entry;
	bool has_entries = dir.GetFirst(&entry, wxEmptyString, wxDIR_DIRS);
	while (has_entries) {
		if (entry != "." && entry != "..") {
			rom_combo_->Append(entry + "/", new wxStringClientData(entry));
		}
		has_entries = dir.GetNext(&entry);
	}

	has_entries = dir.GetFirst(&entry, wxEmptyString, wxDIR_FILES);
	while (has_entries) {
		if (!entry.StartsWith(".") && !entry.Lower().EndsWith(".txt")) {
			rom_combo_->Append(entry, new wxStringClientData(entry));
		}
		has_entries = dir.GetNext(&entry);
	}
}

wxString MachineEditDialog::SelectedRomDir() const
{
	const int sel = rom_combo_->GetSelection();
	if (sel < 0) {
		return wxEmptyString;
	}

	const wxString label = rom_combo_->GetString(sel);
	if (label.StartsWith("(")) {
		return wxEmptyString;
	}

	auto *data = dynamic_cast<wxStringClientData *>(rom_combo_->GetClientObject(sel));
	if (data != nullptr) {
		return data->GetData();
	}

	wxString rom_dir = label;
	if (rom_dir.EndsWith("/")) {
		rom_dir.RemoveLast();
	}
	return rom_dir;
}

void MachineEditDialog::SetRomSelection(const wxString &rom_dir)
{
	for (unsigned i = 0; i < rom_combo_->GetCount(); ++i) {
		auto *data = dynamic_cast<wxStringClientData *>(rom_combo_->GetClientObject(i));
		if (data != nullptr && data->GetData() == rom_dir) {
			rom_combo_->SetSelection(static_cast<int>(i));
			return;
		}
	}
	rom_combo_->SetSelection(0);
}

Model MachineEditDialog::CurrentModelSelection() const
{
	const int sel = model_combo_->GetSelection();
	if (sel >= 0 && sel < Model_MAX) {
		return static_cast<Model>(sel);
	}
	return Model_RPCARM710;
}

void MachineEditDialog::UpdateRomModelCompatibility()
{
	char detail[64] = "";
	char msg[512] = "";
	const Model model = CurrentModelSelection();
	const wxString rom_dir = SelectedRomDir();
	const wxScopedCharBuffer rom_dir_utf8 = rom_dir.utf8_str();

	if (!rom_model_is_compatible(model, rom_dir_utf8.data(), msg, sizeof(msg))) {
		compat_label_->SetLabel(wxString::FromUTF8(msg));
		compat_label_->SetForegroundColour(wxColour(176, 0, 32));
		return;
	}

	const RomAddressing addressing =
		rom_probe_addressing(rom_dir_utf8.data(), detail, sizeof(detail));

	wxString label;
	if (addressing == RomAddressing_32Bit) {
		label = "32-bit ROM - OK for this model";
	} else if (addressing == RomAddressing_26Bit) {
		label = "26-bit ROM - OK for this model";
	} else {
		compat_label_->SetLabel("ROM type unknown - 26-bit CPUs need RISC OS 3.xx ROMs");
		compat_label_->SetForegroundColour(wxColour(27, 94, 32));
		return;
	}

	if (detail[0] != '\0') {
		label += wxString(" (") + wxString::FromUTF8(detail) + wxString(")");
	}

	compat_label_->SetLabel(label);
	compat_label_->SetForegroundColour(wxColour(27, 94, 32));
}

void MachineEditDialog::LoadSettings()
{
	loading_settings_ = true;
	wxFileConfig settings(wxEmptyString, wxEmptyString, config_path_, wxEmptyString, wxCONFIG_USE_RELATIVE_PATH);
	ConfigFileUseGeneralGroup(settings);

	settings.Read("name", &original_name_, wxEmptyString);
	if (original_name_.empty()) {
		original_name_ = wxFileName(config_path_).GetName();
	}
	name_edit_->SetValue(original_name_);

	wxString rom_dir;
	settings.Read("rom_dir", &rom_dir, wxEmptyString);
	SetRomSelection(rom_dir);

	wxString model_name;
	settings.Read("model", &model_name, "RPCSA");
	if (model_name == "RPCARM610") model_name = "RPC610";
	if (model_name == "RPCARM710") model_name = "RPC710";
	if (model_name == "RPCARM810") model_name = "RPC810";
	for (int i = 0; i < Model_MAX; ++i) {
		if (model_name == wxString::FromUTF8(models[i].name_config)) {
			model_combo_->SetSelection(i);
			break;
		}
	}

	long mem_size = 64;
	settings.Read("mem_size", &mem_size, 64L);
	const int mem_values[] = {4, 8, 16, 32, 64, 128, 256};
	for (int i = 0; i < 7; ++i) {
		if (static_cast<long>(mem_values[i]) == mem_size) {
			mem_combo_->SetSelection(i);
			break;
		}
	}

	wxString vram_text;
	settings.Read("vram_size", &vram_text, "2");
	vram_combo_->SetSelection(vram_text == "0" ? 0 : 1);

	long refresh = 60;
	settings.Read("refresh_rate", &refresh, 60L);
	refresh = std::max(20L, std::min(100L, refresh));
	refresh_slider_->SetValue(static_cast<int>(refresh));
	refresh_label_->SetLabel(wxString::Format("%ld Hz", refresh));

	wxString network_type;
	settings.Read("network_type", &network_type, "off");
	const int net_index = network_combo_->FindString(
		network_type == "off" ? "Off" :
		network_type == "nat" ? "NAT" :
		network_type == "ethernetbridging" ? "Ethernet Bridging" : "IP Tunnelling");
	if (net_index != wxNOT_FOUND) {
		network_combo_->SetSelection(net_index);
	}

	wxString bridge;
	settings.Read("bridgename", &bridge, "rpcemu");
	bridge_edit_->SetValue(bridge);
	wxString ip;
	settings.Read("ipaddress", &ip, "172.31.0.1");
	tunnel_edit_->SetValue(ip);

	settings.Read("hd4_path", &hd4_path_, wxEmptyString);

	long cdrom_enabled = 0;
	settings.Read("cdrom_enabled", &cdrom_enabled, 0L);
	cdrom_enabled_ = cdrom_enabled != 0;

	wxCommandEvent dummy;
	OnNetworkChanged(dummy);
	loading_settings_ = false;

	CallAfter([this]() { UpdateRomModelCompatibility(); });

	if (!allow_rename_) {
		name_edit_->Disable();
	}
}

void MachineEditDialog::SaveSettings()
{
	wxFileConfig settings(wxEmptyString, wxEmptyString, config_path_, wxEmptyString, wxCONFIG_USE_RELATIVE_PATH);
	ConfigFileUseGeneralGroup(settings);

	if (!allow_rename_) {
		new_name_ = original_name_;
	} else {
		new_name_ = ConfigPathsSanitizeName(name_edit_->GetValue());
		new_name_.Trim(true).Trim(false);
		if (new_name_.empty()) {
			new_name_ = original_name_;
		}
	}

	const int mem_values[] = {4, 8, 16, 32, 64, 128, 256};
	const int mem_sel = std::max(0, mem_combo_->GetSelection());
	const int vram_sel = std::max(0, vram_combo_->GetSelection());
	const int model_sel = std::max(0, model_combo_->GetSelection());

	const wxString rom_dir = SelectedRomDir();

	wxString network_type = "off";
	const wxString network_label = network_combo_->GetStringSelection();
	if (network_label == "NAT") {
		network_type = "nat";
	} else if (network_label == "Ethernet Bridging") {
		network_type = "ethernetbridging";
	} else if (network_label == "IP Tunnelling") {
		network_type = "iptunnelling";
	}

	settings.Write("name", new_name_);
	settings.Write("rom_dir", rom_dir);
	settings.Write("model", wxString::FromUTF8(models[model_sel].name_config));
	settings.Write("mem_size", wxString::Format("%d", mem_values[mem_sel]));
	settings.Write("vram_size", vram_sel == 0 ? "0" : "2");
	settings.Write("refresh_rate", refresh_slider_->GetValue());
	settings.Write("network_type", network_type);
	settings.Write("bridgename", bridge_edit_->GetValue());
	settings.Write("ipaddress", tunnel_edit_->GetValue());

	if (!settings.Flush()) {
		wxLogError("Failed to save machine configuration to %s", config_path_);
	}

	renamed_ = allow_rename_ && (new_name_ != original_name_);
	ApplySavedSettingsToGlobalConfig(rom_dir, mem_values[mem_sel], vram_sel == 0 ? 0 : 8,
	                                 refresh_slider_->GetValue(),
	                                 network_type == "nat" ? NetworkType_NAT :
	                                 network_type == "ethernetbridging" ? NetworkType_EthernetBridging :
	                                 network_type == "iptunnelling" ? NetworkType_IPTunnelling :
	                                 NetworkType_Off);
}

void MachineEditDialog::ApplySavedSettingsToGlobalConfig(const wxString &rom_dir, int mem_size,
                                                         int vram_internal, int refresh,
                                                         NetworkType network_type)
{
	const wxScopedCharBuffer name_utf8 = new_name_.utf8_str();
	const wxScopedCharBuffer rom_utf8 = rom_dir.utf8_str();
	const wxScopedCharBuffer bridge_utf8 = bridge_edit_->GetValue().utf8_str();
	const wxScopedCharBuffer ip_utf8 = tunnel_edit_->GetValue().utf8_str();
	config_apply_machine_edit(&config, name_utf8.data(), rom_utf8.data(),
	                          static_cast<unsigned>(mem_size),
	                          static_cast<unsigned>(vram_internal), refresh, network_type,
	                          bridge_utf8.data(), ip_utf8.data());
}

wxString MachineEditDialog::CurrentMachineNameForHd() const
{
	if (!allow_rename_) {
		return original_name_;
	}

	wxString name = ConfigPathsSanitizeName(name_edit_->GetValue());
	name.Trim(true).Trim(false);
	if (name.empty()) {
		return original_name_;
	}
	return name;
}

wxString MachineEditDialog::HardDiscFilePath(int drive) const
{
	if (drive == 4 && !hd4_path_.empty() && wxFileName(hd4_path_).IsAbsolute()) {
		return hd4_path_;
	}

	const wxString machine_dir = ConfigPathsMachinesDir() + wxFileName::GetPathSeparator() +
	                             CurrentMachineNameForHd();
	return machine_dir + wxFileName::GetPathSeparator() + wxString::Format("hd%d.hdf", drive);
}

MachineEditDialog::HardDiscInfo MachineEditDialog::QueryHardDiscInfo(int drive) const
{
	HardDiscInfo info;
	info.path = HardDiscFilePath(drive);
	info.uses_custom_path = drive == 4 && !hd4_path_.empty() && wxFileName(hd4_path_).IsAbsolute();

	if (!wxFileExists(info.path)) {
		info.state = HardDiscState::Missing;
		return info;
	}

	const wxULongLong size_bytes = wxFileName::GetSize(info.path);
	info.size_text = FormatHardDiscSize(size_bytes);
	info.modified_text = FormatModifiedTime(info.path);

	if (size_bytes == 0) {
		info.state = HardDiscState::Empty;
		return info;
	}

	if (drive == 5 && cdrom_enabled_) {
		info.state = HardDiscState::Blocked;
		return info;
	}

	info.state = info.uses_custom_path ? HardDiscState::CustomPath : HardDiscState::Ready;
	return info;
}

void MachineEditDialog::ApplyHardDiscPanel(HardDiscPanel &panel, const HardDiscInfo &info)
{
	wxString badge_text;
	wxColour badge_colour = kHdColourMissing;

	switch (info.state) {
	case HardDiscState::Missing:
		badge_text = "Not created";
		badge_colour = kHdColourMissing;
		break;
	case HardDiscState::Empty:
		badge_text = "Empty - will not attach";
		badge_colour = kHdColourEmpty;
		break;
	case HardDiscState::Ready:
		badge_text = wxString::Format("Ready - %s", info.size_text);
		badge_colour = kHdColourReady;
		break;
	case HardDiscState::CustomPath:
		badge_text = wxString::Format("Ready - %s (custom path)", info.size_text);
		badge_colour = kHdColourReady;
		break;
	case HardDiscState::Blocked:
		badge_text = wxString::Format("Unavailable - CD-ROM enabled (%s on disk)", info.size_text);
		badge_colour = kHdColourBlocked;
		break;
	}

	panel.badge->SetLabel(badge_text);
	panel.badge->SetForegroundColour(badge_colour);

	wxString path_text = TruncatePathMiddle(info.path);
	if (!allow_rename_ || CurrentMachineNameForHd() == original_name_) {
		panel.path_label->SetLabel(path_text);
	} else {
		path_text += "  (preview for unsaved name)";
		panel.path_label->SetLabel(path_text);
	}
	if (!info.path.empty()) {
		panel.path_label->SetToolTip(info.path);
	} else {
		panel.path_label->UnsetToolTip();
	}

	if (info.modified_text.empty()) {
		panel.modified_label->SetLabel(wxEmptyString);
	} else {
		panel.modified_label->SetLabel(info.modified_text);
	}

	const bool file_exists = info.state != HardDiscState::Missing;
	panel.delete_btn->Enable(file_exists);
	panel.open_folder_btn->Enable(file_exists);
	panel.create_btn->Enable(true);
}

void MachineEditDialog::UpdateHdStatus()
{
	ApplyHardDiscPanel(hd4_panel_, QueryHardDiscInfo(4));
	ApplyHardDiscPanel(hd5_panel_, QueryHardDiscInfo(5));
}

void MachineEditDialog::ShowHardDiscCreateMenu(int drive)
{
	wxMenu menu;
	menu.Append(ID_HD_CREATE_256_MB, "256 MB");
	menu.Append(ID_HD_CREATE_512_MB, "512 MB");
	menu.Append(ID_HD_CREATE_1_GB, "1 GB");
	menu.Append(ID_HD_CREATE_2_GB, "2 GB");

	menu.Bind(wxEVT_MENU, [this, drive](wxCommandEvent &event) {
		int size_mb = 512;
		switch (event.GetId()) {
		case ID_HD_CREATE_256_MB:
			size_mb = 256;
			break;
		case ID_HD_CREATE_1_GB:
			size_mb = 1024;
			break;
		case ID_HD_CREATE_2_GB:
			size_mb = 2048;
			break;
		default:
			break;
		}
		CreateHardDisc(drive, size_mb);
	});

	wxButton *const create_btn = drive == 4 ? hd4_panel_.create_btn : hd5_panel_.create_btn;
	if (create_btn != nullptr) {
		create_btn->PopupMenu(&menu);
	}
}

void MachineEditDialog::CreateHardDisc(int drive, int size_mb)
{
	const wxString hd_path = HardDiscFilePath(drive);
	const wxString machine_dir = wxFileName(hd_path).GetPath();

	if (wxFileExists(hd_path)) {
		const HardDiscInfo info = QueryHardDiscInfo(drive);
		if (info.state != HardDiscState::Empty) {
			wxMessageBox(wxString::Format("HardDisc %d already exists.", drive), "Create HardDisc",
			             wxOK | wxICON_INFORMATION, this);
			return;
		}
	}

	wxDir::Make(machine_dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	wxFile file(hd_path, wxFile::write);
	if (!file.IsOpened()) {
		wxMessageBox(wxString::Format("Could not create %s", hd_path), "Create HardDisc",
		             wxOK | wxICON_ERROR, this);
		return;
	}

	const wxULongLong bytes = static_cast<wxULongLong>(size_mb) * 1024ULL * 1024ULL;
	file.Seek(static_cast<wxFileOffset>((bytes - 1).GetValue()));
	file.Write("\0", 1);
	file.Close();

	UpdateHdStatus();

	wxString message = wxString::Format("Hard disc %d created (%d MB).", drive, size_mb);
	if (emulator_running_) {
		message += "\n\nUse Machine → Reset (not RISC OS Restart) so RPCEmu reloads the IDE image.";
	}
	wxMessageBox(message, "Create HardDisc", wxOK | wxICON_INFORMATION, this);
}

void MachineEditDialog::DeleteHardDisc(int drive)
{
	const wxString hd_path = HardDiscFilePath(drive);
	if (!wxFileExists(hd_path)) {
		return;
	}

	wxString message = wxString::Format(
	    "Delete HardDisc %d?\n\nAll data on this disc will be lost.", drive);
	if (emulator_running_) {
		message += "\n\nThe emulator is running - this change will take effect after reset.";
	}

	if (wxMessageBox(message, "Delete HardDisc", wxYES_NO | wxICON_WARNING, this) != wxYES) {
		return;
	}

	if (!wxRemoveFile(hd_path)) {
		wxMessageBox(wxString::Format("Could not delete %s", hd_path), "Delete HardDisc",
		             wxOK | wxICON_ERROR, this);
		return;
	}

	UpdateHdStatus();
}

void MachineEditDialog::OpenHardDiscFolder(int drive)
{
	const wxString hd_path = HardDiscFilePath(drive);
	const wxString dir = wxFileName(hd_path).GetPath();
	if (!wxDirExists(dir)) {
		wxMessageBox(wxString::Format("Folder does not exist:\n%s", dir), "Open folder",
		             wxOK | wxICON_INFORMATION, this);
		return;
	}

	if (!wxLaunchDefaultApplication(dir)) {
		wxMessageBox(wxString::Format("Could not open folder:\n%s", dir), "Open folder",
		             wxOK | wxICON_ERROR, this);
	}
}

void MachineEditDialog::OnNetworkChanged(wxCommandEvent &)
{
	const wxString label = network_combo_->GetStringSelection();
	const bool bridge = (label == "Ethernet Bridging");
	bridge_label_->Enable(bridge);
	bridge_edit_->Enable(bridge);
	const bool tunnel = (label == "IP Tunnelling");
	tunnel_label_->Enable(tunnel);
	tunnel_edit_->Enable(tunnel);
}

void MachineEditDialog::OnRomOrModelChanged(wxCommandEvent &)
{
	if (loading_settings_) {
		return;
	}
	UpdateRomModelCompatibility();
}

void MachineEditDialog::OnNameChanged(wxCommandEvent &)
{
	if (loading_settings_) {
		return;
	}
	UpdateHdStatus();
}

void MachineEditDialog::OnOk(wxCommandEvent &)
{
	char msg[512];
	const Model model = CurrentModelSelection();
	const wxString rom_dir = SelectedRomDir();
	const wxScopedCharBuffer rom_dir_utf8 = rom_dir.utf8_str();
	if (!rom_model_is_compatible(model, rom_dir_utf8.data(), msg, sizeof(msg))) {
		wxMessageBox(wxString::FromUTF8(msg), "ROM compatibility", wxOK | wxICON_WARNING, this);
		return;
	}
	SaveSettings();
	EndModal(wxID_OK);
}
