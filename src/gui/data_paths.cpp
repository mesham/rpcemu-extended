#include "data_paths.h"

#include "config_paths.h"

#include <cstdlib>
#include <string>

#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/stdpaths.h>

extern "C" {
#include "rpcemu.h"
}

#ifndef RPCEMU_INSTALL_DATADIR
#define RPCEMU_INSTALL_DATADIR "/usr/share/rpcemu"
#endif

static wxString NormalizeDirPath(const wxString &path)
{
	if (path.empty()) {
		return wxEmptyString;
	}

	wxFileName fn;
	fn.AssignDir(path);
	fn.Normalize(wxPATH_NORM_ALL);
	return fn.GetPathWithSep();
}

static bool HasConfigsDir(const wxString &base)
{
	return wxDirExists(NormalizeDirPath(base) + "configs");
}

static wxString UserDataRoot()
{
	const char *xdg_data_home = getenv("XDG_DATA_HOME");
	wxString base;
	if (xdg_data_home && xdg_data_home[0] != '\0') {
		base = wxString::FromUTF8(xdg_data_home);
	} else {
		const char *home = getenv("HOME");
		if (home && home[0] != '\0') {
			base = wxString::FromUTF8(home) + wxFileName::GetPathSeparator() + ".local" +
			       wxFileName::GetPathSeparator() + "share";
		} else {
			base = ".local/share";
		}
	}
	return NormalizeDirPath(base + wxFileName::GetPathSeparator() + "rpcemu");
}

static std::string DirPathForCore(const wxString &dir)
{
	return std::string(NormalizeDirPath(dir).utf8_str().data());
}

static void SeedFileIfMissing(const wxString &src, const wxString &dst)
{
	if (!wxFileExists(src) || wxFileExists(dst)) {
		return;
	}
	wxCopyFile(src, dst, false);
}

static bool SeedUserDataDir(const wxString &resource_dir, const wxString &user_dir)
{
	const wxString resource = NormalizeDirPath(resource_dir);
	const wxString user = NormalizeDirPath(user_dir);
	if (user.empty()) {
		return false;
	}

	if (!wxDir::Make(user, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
		return false;
	}

	if (resource.empty() || resource == user) {
		return ConfigPathsEnsureDataLayout();
	}

	const wxString user_configs = user + "configs";
	const wxString resource_configs = resource + "configs";
	if (wxDirExists(resource_configs)) {
		if (!wxDir::Make(user_configs, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
			return false;
		}
		if (!wxFileExists(user_configs + wxFileName::GetPathSeparator() + "Default.cfg")) {
			if (!ConfigPathsCopyDirectory(resource_configs, user_configs)) {
				return false;
			}
		}
	}

	const wxString user_machine = user + "machines" + wxFileName::GetPathSeparator() + "Default";
	const wxString resource_machine = resource + "machines" + wxFileName::GetPathSeparator() + "Default";
	if (wxDirExists(resource_machine) && !wxDirExists(user_machine)) {
		if (!ConfigPathsCopyDirectory(resource_machine, user_machine)) {
			return false;
		}
	}

	const wxString user_roms = user + "roms";
	if (!wxDir::Make(user_roms, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
		return false;
	}
	SeedFileIfMissing(resource + "roms" + wxFileName::GetPathSeparator() + "roms.txt",
	                  user_roms + wxFileName::GetPathSeparator() + "roms.txt");

	const wxString user_netroms = user + "netroms";
	const wxString resource_netroms = resource + "netroms";
	if (wxDirExists(resource_netroms)) {
		if (!wxDir::Make(user_netroms, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) {
			return false;
		}
		SeedFileIfMissing(resource_netroms + wxFileName::GetPathSeparator() + "EtherRPCEm,ffa",
		                  user_netroms + wxFileName::GetPathSeparator() + "EtherRPCEm,ffa");
	}

	return ConfigPathsEnsureDataLayout();
}

static wxString EnvVar(const char *name)
{
	const char *value = getenv(name);
	if (!value || value[0] == '\0') {
		return wxEmptyString;
	}
	return wxString::FromUTF8(value);
}

void InitRpcemuPaths()
{
	wxString resource_dir;
	wxString user_dir;

	const wxString env_datadir = EnvVar("RPCEMU_DATADIR");
	if (!env_datadir.empty()) {
		user_dir = NormalizeDirPath(env_datadir);
		resource_dir = user_dir;
	} else {
		const wxString env_resource = EnvVar("RPCEMU_RESOURCE_DIR");
		const wxString exe_dir = NormalizeDirPath(wxPathOnly(wxStandardPaths::Get().GetExecutablePath()));
		const wxString cwd = NormalizeDirPath(wxGetCwd());
		const wxString install_dir = NormalizeDirPath(wxString(RPCEMU_INSTALL_DATADIR, wxConvUTF8));

		if (!env_resource.empty()) {
			resource_dir = NormalizeDirPath(env_resource);
			user_dir = UserDataRoot();
		} else if (HasConfigsDir(exe_dir)) {
			resource_dir = exe_dir;
			user_dir = exe_dir;
		} else if (HasConfigsDir(cwd)) {
			resource_dir = cwd;
			user_dir = cwd;
		} else if (HasConfigsDir(install_dir)) {
			resource_dir = install_dir;
			user_dir = UserDataRoot();
		} else if (wxDirExists("/usr/share/rpcemu/configs")) {
			resource_dir = "/usr/share/rpcemu/";
			user_dir = UserDataRoot();
		} else {
			resource_dir = cwd.empty() ? "./" : cwd;
			user_dir = resource_dir;
		}
	}

	if (user_dir.empty()) {
		user_dir = UserDataRoot();
	}
	if (resource_dir.empty()) {
		resource_dir = user_dir;
	}

	rpcemu_set_datadir(DirPathForCore(user_dir).c_str());
	rpcemu_set_resourcedir(DirPathForCore(resource_dir).c_str());

	if (!SeedUserDataDir(resource_dir, user_dir)) {
		wxLogWarning("RPCEmu could not fully prepare the user data directory:\n%s",
		             user_dir);
	}
}
