
#ifndef NANA_FILESYSTEM_FS_UTILITY_HPP
#define NANA_FILESYSTEM_FS_UTILITY_HPP

#include <nana/deploy.hpp>
#include <ctime>

namespace nana
{
namespace filesystem
{
	struct error
	{
		enum
		{
			none = 0
		};
	};

	struct attribute
	{
		long long bytes;
		bool is_directory;
		tm modified;
	};

	bool modified_file_time(const ::std::string& file, struct tm&);

	std::wstring path_user();

	bool rmfile(const char* file_utf8);
	bool rmdir(const char* dir, bool fails_if_not_empty);

}//end namespace filesystem
}//end namespace nana

#endif
