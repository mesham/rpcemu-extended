#ifndef GUI_PREFERENCES_H
#define GUI_PREFERENCES_H

#include <string>
#include <vector>

static const int MaxRecentMachines = 5;
static const int MaxRecentFloppies = 10;
static const int MaxRecentCDROMs = 10;

std::vector<std::string> GetRecentMachines();
void AddRecentMachine(const std::string &machine_name);
void ClearRecentMachines();

std::vector<std::string> GetRecentFloppies();
void AddRecentFloppy(const std::string &path);
void ClearRecentFloppies();

std::vector<std::string> GetRecentCDROMs();
void AddRecentCDROM(const std::string &path);
void ClearRecentCDROMs();

#endif
