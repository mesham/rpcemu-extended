#include "gui_preferences.h"

#include <algorithm>
#include <sstream>

#include <wx/config.h>
#include <wx/tokenzr.h>

namespace {

wxConfig *
OpenPreferences()
{
	return new wxConfig("RPCEmu", "RPCEmu");
}

std::vector<std::string>
ReadRecentList(const char *key, int max_entries)
{
	std::vector<std::string> entries;
	wxConfig *config = OpenPreferences();

	wxString value;
	if (config->Read(key, &value, wxEmptyString) && !value.empty()) {
		wxStringTokenizer tok(value, "\n", wxTOKEN_STRTOK);
		while (tok.HasMoreTokens() && static_cast<int>(entries.size()) < max_entries) {
			const wxString token = tok.GetNextToken();
			if (!token.empty()) {
				entries.emplace_back(token.utf8_string());
			}
		}
	}

	delete config;
	return entries;
}

void
WriteRecentList(const char *key, const std::vector<std::string> &entries)
{
	wxConfig *config = OpenPreferences();

	std::ostringstream joined;
	for (size_t i = 0; i < entries.size(); i++) {
		if (i > 0) {
			joined << '\n';
		}
		joined << entries[i];
	}

	config->Write(key, wxString::FromUTF8(joined.str().c_str()));
	config->Flush();

	delete config;
}

void
AddRecentEntry(const char *key, const std::string &value, int max_entries)
{
	if (value.empty()) {
		return;
	}

	std::vector<std::string> entries = ReadRecentList(key, max_entries);
	entries.erase(std::remove(entries.begin(), entries.end(), value), entries.end());
	entries.insert(entries.begin(), value);

	if (static_cast<int>(entries.size()) > max_entries) {
		entries.resize(static_cast<size_t>(max_entries));
	}

	WriteRecentList(key, entries);
}

} // namespace

std::vector<std::string>
GetRecentMachines()
{
	return ReadRecentList("recentMachines", MaxRecentMachines);
}

void
AddRecentMachine(const std::string &machine_name)
{
	AddRecentEntry("recentMachines", machine_name, MaxRecentMachines);
}

void
ClearRecentMachines()
{
	WriteRecentList("recentMachines", {});
}

std::vector<std::string>
GetRecentFloppies()
{
	return ReadRecentList("recentFloppies", MaxRecentFloppies);
}

void
AddRecentFloppy(const std::string &path)
{
	AddRecentEntry("recentFloppies", path, MaxRecentFloppies);
}

void
ClearRecentFloppies()
{
	WriteRecentList("recentFloppies", {});
}

std::vector<std::string>
GetRecentCDROMs()
{
	return ReadRecentList("recentCDROMs", MaxRecentCDROMs);
}

void
AddRecentCDROM(const std::string &path)
{
	AddRecentEntry("recentCDROMs", path, MaxRecentCDROMs);
}

void
ClearRecentCDROMs()
{
	WriteRecentList("recentCDROMs", {});
}
