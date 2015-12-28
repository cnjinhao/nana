
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

	bool file_attrib(const ::std::string& file, attribute&);
	//long long filesize(const nana::string& file);	//deprecated

	//bool mkdir(const ::std::string& dir, bool & if_exist);	//deprecated
	bool modified_file_time(const ::std::string& file, struct tm&);

	std::wstring path_user();
	std::wstring path_current();

	bool rmfile(const char* file_utf8);
	bool rmdir(const char* dir, bool fails_if_not_empty);
	nana::string root(const nana::string& path);

	/*
	class path
	{
	public:
		struct type
		{	enum{not_exist, file, directory};
		};

		path();
		path(const nana::string&);

		bool empty() const;
		path root() const;
		int what() const;

		nana::string name() const;
	private:
#if defined(NANA_WINDOWS)
		nana::string text_;
#else
		std::string text_;
#endif
	};
	*/
	
}//end namespace filesystem
}//end namespace nana

#endif
