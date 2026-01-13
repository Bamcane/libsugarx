#ifndef LIBSUGARX_PATH_H
#define LIBSUGARX_PATH_H

#include <cstdlib>
#include <filesystem>

namespace libsugarx
{
	/*
	returns:
	home directory or local directory.
	*/
	std::filesystem::path get_home_directory() noexcept
	{
		if(const char *home = std::getenv("USERPROFILE"))
			if(home[0]) return home;
		if(const char *home = std::getenv("HOME"))
			if(home[0]) return home;
		const char *drive = std::getenv("HOMEDRIVE");
		const char *path = std::getenv("HOMEPATH");
		if(drive && path && drive[0] && path[0]) return std::filesystem::path(drive) / path;

		return std::filesystem::current_path();
	}
	/*
	returns:
	home data directory or local directory.
	*/
	std::filesystem::path get_data_home_directory() noexcept
	{
		if(const char *data_home = std::getenv("LOCALAPPDATA"))
			if(data_home[0]) return data_home;
		if(const char *data_home = std::getenv("XDG_DATA_HOME"))
			if(data_home[0]) return data_home;
#ifdef __APPLE__
		if(const char *home = std::getenv("HOME"))
			if(home[0]) return std::filesystem::path(home) / "Library" / "Application Support";
#else
		if(const char *home = std::getenv("HOME"))
			if(home[0]) return std::filesystem::path(home) / ".local" / "share";
#endif

		return std::filesystem::current_path();
	}
}; // namespace libsugarx

#endif // LIBSUGARX_PATH_H
