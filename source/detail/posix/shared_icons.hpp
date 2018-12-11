#ifndef NANA_DETAIL_SHARED_ICONS_INCLUDED
#define NANA_DETAIL_SHARED_ICONS_INCLUDED

#include <string>
#include <fstream>

namespace nana
{
	namespace detail
	{
		class shared_icons
		{
		public:
			shared_icons();

			std::string cursor(const std::string& name);
		private:
			std::string _m_read(const std::string& category, const std::string& key);
		private:
			std::string path_;
			std::ifstream ifs_;
		};

	}//end namespace detail

}//end namespace nana

#endif