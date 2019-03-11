#ifndef NANA_DETAIL_THEME_INCLUDED
#define NANA_DETAIL_THEME_INCLUDED

#include <string>
#include <map>
#include <vector>
#include <fstream>

namespace nana
{
	namespace detail
	{
		class theme
		{
		public:
			theme();

			std::string cursor(const std::string& name) const;
			std::string icon(const std::string& name, std::size_t size_wanted) const;
		private:
			std::string path_;
			mutable std::ifstream ifs_;

			mutable std::map<std::string, std::vector<std::pair<std::size_t, std::string>>> iconcache_;
		};

	}//end namespace detail

}//end namespace nana

#endif