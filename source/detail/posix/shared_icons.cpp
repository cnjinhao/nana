#include "shared_icons.hpp"

namespace nana
{
	namespace detail
	{
		shared_icons::shared_icons():
			path_("/usr/share/icons/"),
			ifs_("/usr/share/icons/default/index.theme")
		{
		}

		std::string shared_icons::cursor(const std::string& name)
		{
			auto theme = _m_read("Icon Theme", "Inherits");
			return path_ + theme + "/cursors/" + name;
		}
		
		std::string shared_icons::_m_read(const std::string& category, const std::string& key)
		{
			ifs_.seekg(0, std::ios::beg);

			bool found_cat = false;
			while(ifs_.good())
			{
				std::string text;
				std::getline(ifs_, text);

				if(0 == text.find('['))
				{
					if(found_cat)
						break;

					if(text.find(category + "]") != text.npos)
					{
						found_cat = true;
					}
				}
				else if(found_cat && (text.find(key + "=") == 0))
				{
					return text.substr(key.size() + 1);
				}
			}

			return {};
		}

	}

}