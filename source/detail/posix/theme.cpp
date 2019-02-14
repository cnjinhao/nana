#include <nana/c++defines.hpp>
#if defined(NANA_POSIX) && defined(NANA_X11)
#include "theme.hpp"
#include <nana/filesystem/filesystem.hpp>
#include <algorithm>
#include <vector>

namespace nana
{
	namespace detail
	{
		static int gschema_override_priority(const std::string& filename)
		{
			if(filename.size() < 3)
				return -1;

			auto str = filename.substr(0, 2);
			if('0' <= str[0] && str[0] <= '9' && '0' <= str[1] && str[1] <= '9')
				return std::stoi(str);
			
			return 0;
		}

		//Removes the wrap of ' and " character.
		std::string remove_decoration(const std::string& primitive_value)
		{
			auto pos = primitive_value.find_first_of("'\"");
			if(pos == primitive_value.npos)
				return primitive_value;

			auto endpos = primitive_value.find(primitive_value[pos], pos + 1);
			if(endpos == primitive_value.npos)
				return primitive_value;

			return primitive_value.substr(pos + 1, endpos - pos - 1);			
		}

		std::string find_value(std::ifstream& ifs, const std::string& category, const std::string& key)
		{
			ifs.seekg(0, std::ios::beg);

			std::string dec_categ = "[" + category + "]";

			bool found_cat = false;
			while(ifs.good())
			{
				std::string text;
				std::getline(ifs, text);

				if((text.size() > 2) && ('[' == text[0]))
				{
					if(found_cat)
						break;

					found_cat = (text == dec_categ);
				}
				else if(found_cat && (text.find(key + "=") == 0))
				{
					return remove_decoration(text.substr(key.size() + 1));
				}
			}
			return {};
		}

		std::vector<std::string> split_value(const std::string& value_string)
		{
			std::vector<std::string> values;

			std::size_t start_pos = 0;
			while(start_pos != value_string.npos)
			{
				auto pos = value_string.find(',', start_pos);
				if(value_string.npos == pos)
				{
					if(start_pos < value_string.size())
						values.emplace_back(value_string.substr(start_pos));
					break;
				}
					
				values.emplace_back(value_string.substr(start_pos, pos - start_pos));
					
				start_pos = value_string.find_first_not_of(',', pos);
			}
			return values;
		}

		namespace fs = std::filesystem;
		std::string find_gnome_theme_name()
		{
			try
			{
				//Searches all the gschema override files
				std::vector<std::string> overrides;
				for(fs::directory_iterator i{"/usr/share/glib-2.0/schemas"}, end; i != end; ++i)
				{
					auto filename = i->path().filename().string();
					if(filename.size() > 17 && filename.substr(filename.size() - 17) == ".gschema.override")
					{
						auto priority = gschema_override_priority(filename);
						if(priority < 0)
							continue;
						
						auto i = std::find_if(overrides.cbegin(), overrides.cend(), [priority](const std::string& ovrd){
							return (priority > gschema_override_priority(ovrd));
						});

						overrides.emplace(i, std::move(filename));
						//overrides.insert(i, filename);
					}
				}

				//Searches the org.gnome.desktop.interface in override files.
				for(auto & gschema_override : overrides)
				{
					std::ifstream ifs{"/usr/share/glib-2.0/schemas/" + gschema_override};
					auto value = find_value(ifs, "org.gnome.desktop.interface", "icon-theme");
					if(!value.empty())
						return value;
				}

				//Return the value from org.gnome.desktop.interface.gschema.xml
				fs::path xml = "/usr/share/glib-2.0/schemas/org.gnome.desktop.interface.gschema.xml";
				auto bytes = fs::file_size(xml);
				if(0 == bytes)
					return {};
				
				std::ifstream xml_ifs{"/usr/share/glib-2.0/schemas/org.gnome.desktop.interface.gschema.xml", std::ios::binary};
				if(xml_ifs)
				{
					std::string data;
					data.resize(bytes);

					xml_ifs.read(&data.front(), bytes);

					auto pos = data.find("\"icon-theme\"");
					if(pos != data.npos)
					{

						pos = data.find("<default>", pos + 22);
						if(pos != data.npos)
						{
							pos += 9;
							auto endpos = data.find("</default>", pos);
							if(endpos != data.npos)
							{
								return remove_decoration(data.substr(pos, endpos - pos));
							}
						}
					}
				}
			}
			catch(...){}

			return {};
		}


		std::string find_kde_theme_name()
		{
			auto home = getenv("HOME");
			if(home)
			{
				fs::path kdeglobals{home};
				kdeglobals /= ".kde/share/config/kdeglobals";

				std::error_code err;
				if(fs::exists(kdeglobals, err))
				{
					std::ifstream ifs{kdeglobals};
					return find_value(ifs, "Icons", "Theme");
				}
			}
			return {};
		}

		std::string find_theme_name()
		{
			auto name = find_kde_theme_name();

			if(name.empty())
				return find_gnome_theme_name();

			return name;
		}


		class icon_theme
		{
		public:
			icon_theme(const std::string& name):
				theme_name_(name),
				ifs_("/usr/share/icons/" + name + "/index.theme")
			{
				//First of all, read the Inherits and Directories
				inherits_ = split_value(find_value(ifs_, "Icon Theme", "Inherits"));
				directories_ = split_value(find_value(ifs_, "Icon Theme", "Directories"));
				
			}

			std::string find(const std::string& name, std::size_t size_wanted) const
			{
				namespace fs = std::filesystem;
				//candidates
				std::vector<std::pair<std::string,int>> first, second, third;

				fs::path theme_path = "/usr/share/icons/" + theme_name_;

				std::string base_path = "/usr/share/icons/" + theme_name_ + "/";
				std::string filename = "/" + name + ".png";

				std::error_code err;
				for(auto & dir : directories_)
				{
					if(!fs::exists(theme_path / dir / (name + ".png"), err))
						continue;

					auto size = find_value(ifs_, dir, "Size");
					auto type = find_value(ifs_, dir, "Type");
					auto scaled = find_value(ifs_, dir, "Scale");

					if(size.empty() || ("Fixed" !=  type))
						continue;

					int int_size = std::stoi(size);

					if(!scaled.empty())
						int_size *= std::stoi(scaled);

					auto distance = std::abs(static_cast<int>(size_wanted) - int_size);

					if(0 == distance)
					{
						if(scaled.empty() || scaled == "1")
							return base_path + dir + filename;
						else
							first.emplace_back(dir, 0);
					}
					else
					{
						if(scaled.empty() || scaled == "1")
							second.emplace_back(dir, distance);
						else
							third.emplace_back(dir, distance);
					}
				}

				using pair_type = std::pair<std::string,int>;
				auto comp = [](const pair_type& a, const pair_type& b){
					return a.second < b.second;
				};

				std::sort(first.begin(), first.end(), comp);
				std::sort(second.begin(), second.end(), comp);
				std::sort(third.begin(), third.end(), comp);

				std::string closer_dir;
				if(!first.empty())
					closer_dir = first.front().first;
				else if(!second.empty())
					closer_dir = second.front().first;
				else if(!third.empty())
					closer_dir = third.front().first;


				if(closer_dir.empty())
				{
					for(auto & inh : inherits_)
					{
						auto dir = icon_theme{inh}.find(name, size_wanted);
						if(!dir.empty())
							return dir;
					}

					//Avoid recursively traverse directory for hicolor if current theme name is hicolor
					if("hicolor" == theme_name_)
						return {};

					return icon_theme{"hicolor"}.find(name, size_wanted);
				}
				
				return base_path + closer_dir + filename;
			}
		private:
			const std::string theme_name_;
			mutable std::ifstream ifs_;
			std::vector<std::string> inherits_;
			std::vector<std::string> directories_;
		};

		theme::theme():
			path_("/usr/share/icons/"),
			ifs_("/usr/share/icons/default/index.theme")
		{
		}

		std::string theme::cursor(const std::string& name) const
		{
			auto theme = find_value(ifs_, "Icon Theme", "Inherits");
			return path_ + theme + "/cursors/" + name;
		}

		std::string theme::icon(const std::string& name, std::size_t size_wanted) const
		{
			//Lookup in cache
			auto i = iconcache_.find(name);
			if(i != iconcache_.end())
			{
				for(auto & p : i->second)
				{
					if(p.first == size_wanted)
						return p.second;
				}
			}

			//Cache is missed.
			auto file = icon_theme{find_theme_name()}.find(name, size_wanted);
			if(!file.empty())
				iconcache_[name].emplace_back(size_wanted, file);

			return file;
		}

	}

}
#endif