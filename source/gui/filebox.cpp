/*
*	Filebox
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/filebox.cpp
*/

#include <iostream>

#include <nana/gui.hpp>
#include <nana/gui/filebox.hpp>
#include <nana/filesystem/filesystem_ext.hpp>

#if defined(NANA_WINDOWS)
#	include <windows.h>
#	include "../detail/mswin/platform_spec.hpp"
#	ifndef NANA_MINGW	//<Shobjidl.h> isn't supported well on MinGW
#		include <Shobjidl.h>
#	else
#		include <Shlobj.h>
#	endif
#elif defined(NANA_POSIX)
#	include <nana/gui/widgets/label.hpp>
#	include <nana/gui/widgets/button.hpp>
#	include <nana/gui/widgets/listbox.hpp>
#	include <nana/gui/widgets/categorize.hpp>
#	include <nana/gui/widgets/textbox.hpp>
#	include <nana/gui/widgets/treebox.hpp>
#	include <nana/gui/widgets/combox.hpp>
#	include <nana/gui/place.hpp>
#	include <stdexcept>
#	include <algorithm>
#	include "../detail/posix/theme.hpp"
#endif

#include <iostream> //debug

namespace fs = std::filesystem;
namespace fs_ext = nana::filesystem_ext;

namespace nana
{
#if defined(NANA_POSIX)
	class filebox_implement
		: public form
	{
		struct item_fs
		{
			std::string name;
			::tm modified_time;
			bool directory;
			long long bytes;

			friend listbox::iresolver& operator>>(listbox::iresolver& ires, item_fs& m)
			{
				std::wstring type;
				ires>>m.name>>type>>type;
				m.directory = (to_utf8(type) == internationalization()("NANA_FILEBOX_DIRECTORY"));
				return ires;
			}

			friend listbox::oresolver& operator<<(listbox::oresolver& ores, const item_fs& item)
			{
				std::wstringstream tm;
				tm<<(item.modified_time.tm_year + 1900)<<'-';    /// \todo : use nana::filesystem_ext:: pretty_file_date
				_m_add(tm, item.modified_time.tm_mon + 1)<<'-';
				_m_add(tm, item.modified_time.tm_mday)<<' ';

				_m_add(tm, item.modified_time.tm_hour)<<':';
				_m_add(tm, item.modified_time.tm_min)<<':';
				_m_add(tm, item.modified_time.tm_sec);

				internationalization i18n;

				ores<<item.name<<tm.str();
				if(!item.directory)
				{
					auto pos = item.name.find_last_of('.');
					if(pos != item.name.npos && (pos + 1 < item.name.size()))
						ores<<item.name.substr(pos + 1);
					else
						ores<<i18n("NANA_FILEBOX_FILE");

					ores<<_m_trans(item.bytes);
				}
				else
					ores << i18n("NANA_FILEBOX_DIRECTORY");
				return ores;
			}

		private:
			static std::wstringstream& _m_add(std::wstringstream& ss, unsigned v)
			{
				if(v < 10)
					ss<<L'0';
				ss<<v;
				return ss;
			}

			static std::string _m_trans(std::size_t bytes)  /// \todo : use nana::filesystem_ext::pretty_file_size
			{
				const char * ustr[] = {" KB", " MB", " GB", " TB"};
				std::stringstream ss;
				if(bytes >= 1024)
				{
					double cap = bytes / 1024.0;
					std::size_t uid = 0;
					while((cap >= 1024.0) && (uid < (sizeof(ustr) / sizeof(char*))))
					{
						cap /= 1024.0;
						++uid;
					}
					ss<<cap;
					auto s = ss.str();
					auto pos = s.find('.');
					if(pos != s.npos)
					{
						if(pos + 2 < s.size())
							s.erase(pos + 2);
					}

					return s + ustr[uid];
				}
				internationalization i18n;
				ss<<bytes<<" "<<i18n("NANA_FILEBOX_BYTES");
				return ss.str();
			}
		};

		struct pred_sort_fs
		{
			bool operator()(const item_fs& a, const item_fs& b) const
			{
				return ((a.directory != b.directory) && a.directory);
			}
		};
	public:
		enum class mode
		{
			open_file,	//choose an existing file.
			write_file, //choose a filename, it can be specified a new filename which doesn't exist.
			open_directory, //choose an existing directory.
		};

		struct kind
		{
			enum t{none, filesystem};
		};

		typedef treebox::item_proxy item_proxy;
	public:

		filebox_implement(window owner, mode dialog_mode, const std::string& title, bool pick_directory, bool allow_multi_select):
			form(owner, API::make_center(owner, 630, 440)),
			pick_directory_(pick_directory),
			mode_(dialog_mode)
		{
			images_.folder.open(theme_.icon("folder", 16));
			images_.file.open(theme_.icon("empty", 16));
			images_.exec.open(theme_.icon("exec", 16));
			images_.package.open(theme_.icon("package", 16));
			images_.text.open(theme_.icon("text", 16));
			images_.xml.open(theme_.icon("text-xml", 16));
			images_.image.open(theme_.icon("image", 16));
			images_.pdf.open(theme_.icon("application-pdf", 16));

			internationalization i18n;
			path_.create(*this);
			path_.splitstr("/");
			path_.events().selected.connect_unignorable([this](const arg_categorize<int>&)
			{
				auto path = path_.caption();
				auto root = path.substr(0, path.find('/'));
				if(root == "HOME")
					path.replace(0, 4, fs_ext::path_user().native());
				else if(root == "FILESYSTEM")
					path.erase(0, 10);
				else
					throw std::runtime_error("Nana.GUI.Filebox: Wrong categorize path");

				if(path.size() == 0) path = "/";    /// \todo : use nana::filesystem_ext::def_rootstr?
				_m_enter_folder(path);
			});


			filter_.create(*this);
			filter_.multi_lines(false);
			filter_.tip_string(i18n("NANA_FILEBOX_FILTER"));

			filter_.events().key_release.connect_unignorable([this](const arg_keyboard&)
			{
				_m_list_fs();
			});

			btn_folder_.create(*this);
			btn_folder_.i18n(i18n_eval("NANA_FILEBOX_NEW_FOLDER_SHORTKEY"));

			btn_folder_.events().click.connect_unignorable([this](const arg_click&)
			{
				form fm(this->handle(), API::make_center(*this, 300, 35));
				fm.i18n(i18n_eval("NANA_FILEBOX_NEW_FOLDER_CAPTION"));

				textbox folder(fm, nana::rectangle(5, 5, 160, 25));
				folder.multi_lines(false);

				button btn(fm, nana::rectangle(170, 5, 60, 25));
				btn.i18n(i18n_eval("NANA_BUTTON_CREATE"));

				btn.events().click.connect_unignorable(folder_creator(*this, fm, folder));

				button btn_cancel(fm, nana::rectangle(235, 5, 60, 25));
				btn_cancel.i18n(i18n_eval("NANA_BUTTON_CANCEL"));

				btn_cancel.events().click.connect_unignorable([&fm](const arg_click&)
				{
					fm.close();
				});
				API::modal_window(fm);
			});

			tree_.create(*this);

			//Configure treebox icons
			auto & fs_icons = tree_.icon("icon-fs");
			fs_icons.normal.open(theme_.icon("drive-harddisk", 16));

			auto & folder_icons = tree_.icon("icon-folder");
			folder_icons.normal.open(theme_.icon("folder", 16));
			folder_icons.expanded.open(theme_.icon("folder-open", 16));

			tree_.icon("icon-home").normal.open(theme_.icon("folder_home", 16));

			ls_file_.create(*this);
			ls_file_.append_header(i18n("NANA_FILEBOX_HEADER_NAME"), 190);
			ls_file_.append_header(i18n("NANA_FILEBOX_HEADER_MODIFIED"), 145);
			ls_file_.append_header(i18n("NANA_FILEBOX_HEADER_TYPE"), 80);
			ls_file_.append_header(i18n("NANA_FILEBOX_HEADER_SIZE"), 70);


			auto fn_list_handler = [this](const arg_mouse& arg){
				if(event_code::mouse_down == arg.evt_code)
				{
					selection_.is_deselect_delayed = true;
				}
				else if(event_code::mouse_up == arg.evt_code)
				{
					selection_.is_deselect_delayed = false;

					if(_m_sync_with_selection())
						_m_display_target_filenames();
				}
				else if(event_code::mouse_move == arg.evt_code)
				{
					if(arg.left_button)
						selection_.is_deselect_delayed = false;
				}
				else if(event_code::dbl_click == arg.evt_code)
					_m_list_dbl_clicked();
			};

			ls_file_.events().dbl_click.connect_unignorable(fn_list_handler);
			ls_file_.events().mouse_down.connect_unignorable(fn_list_handler);
			ls_file_.events().mouse_up.connect_unignorable(fn_list_handler);
			ls_file_.events().mouse_move.connect_unignorable(fn_list_handler);

			ls_file_.events().selected.connect_unignorable([this](const arg_listbox& arg){
				_m_select_file(arg.item);
			});

			ls_file_.set_sort_compare(0, [](const std::string& a, nana::any* fs_a, const std::string& b, nana::any* fs_b, bool reverse) -> bool
				{
					int dira = any_cast<item_fs>(fs_a)->directory ? 1 : 0;
					int dirb = any_cast<item_fs>(fs_b)->directory ? 1 : 0;
					if(dira != dirb)
						return (reverse ? dira < dirb : dira > dirb);

					std::size_t seek_a = 0;
					std::size_t seek_b = 0;

					while(true)
					{
						std::size_t pos_a = a.find_first_of("0123456789", seek_a);
						std::size_t pos_b = b.find_first_of("0123456789", seek_b);

						if((pos_a != a.npos) && (pos_a == pos_b))
						{
							nana::cistring text_a = a.substr(seek_a, pos_a - seek_a).data();
							nana::cistring text_b = b.substr(seek_b, pos_b - seek_b).data();

							if(text_a != text_b)
								return (reverse ? text_a > text_b : text_a < text_b);

							std::size_t end_a = a.find_first_not_of("0123456789", pos_a + 1);
							std::size_t end_b = b.find_first_not_of("0123456789", pos_b + 1);

							auto num_a = a.substr(pos_a, end_a != a.npos ? end_a - pos_a : a.npos);
							auto num_b = b.substr(pos_b, end_b != b.npos ? end_b - pos_b : b.npos);

							if(num_a != num_b)
							{
								double ai = std::stod(num_a, 0);
								double bi = std::stod(num_b, 0);
								if(ai != bi)
									return (reverse ? ai > bi : ai < bi);
							}

							seek_a = end_a;
							seek_b = end_b;
						}
						else
							break;
					}
					if(seek_a == a.npos)
						seek_a = 0;
					if(seek_b == b.npos)
						seek_b = 0;

					nana::cistring cia = a.data();
					nana::cistring cib = b.data();
					if(seek_a == seek_b && seek_a == 0)
						return (reverse ? cia > cib : cia < cib);
					return (reverse ? cia.substr(seek_a) > cib.substr(seek_b) : cia.substr(seek_a) < cib.substr(seek_b));
				});
			ls_file_.set_sort_compare(2, [](const std::string& a, nana::any* anyptr_a, const std::string& b, nana::any* anyptr_b, bool reverse) -> bool
				{
					int dir1 = any_cast<item_fs>(anyptr_a)->directory ? 1 : 0;
					int dir2 = any_cast<item_fs>(anyptr_b)->directory ? 1 : 0;
					if(dir1 != dir2)
						return (reverse ? dir1 < dir2 : dir1 > dir2);

					return (reverse ? a > b : a < b);
				});
			ls_file_.set_sort_compare(3, [](const std::string&, nana::any* anyptr_a, const std::string&, nana::any* anyptr_b, bool reverse) -> bool
				{
					item_fs * fsa = any_cast<item_fs>(anyptr_a);
					item_fs * fsb = any_cast<item_fs>(anyptr_b);
					return (reverse ? fsa->bytes > fsb->bytes : fsa->bytes < fsb->bytes);
				});

			lb_file_.create(*this);

			const char* idstr = (mode::open_directory == dialog_mode? "NANA_FILEBOX_DIRECTORY_COLON" : "NANA_FILEBOX_FILE_COLON");
			lb_file_.i18n(i18n_eval(idstr));
			lb_file_.text_align(align::right, align_v::center);


			tb_file_.create(*this);
			tb_file_.multi_lines(false);

			tb_file_.events().key_char.connect_unignorable([this](const arg_keyboard& arg)
			{
				allow_fall_back_ = false;
				if(arg.key == nana::keyboard::enter)
					_m_try_select(tb_file_.caption());
			});

			//Don't create the combox for choose a file extension if the dialog is used for picking a directory.
			if(!pick_directory)
			{
				cb_types_.create(*this);
				cb_types_.editable(false);
				cb_types_.events().selected.connect_unignorable([this](const arg_combox&){ _m_list_fs(); });
			}

			btn_ok_.create(*this);
			btn_ok_.i18n(i18n_eval("NANA_BUTTON_OK_SHORTKEY"));

			btn_ok_.events().click.connect_unignorable([this](const arg_click&)
			{
				_m_try_select(tb_file_.caption());
			});

			btn_cancel_.create(*this);
			btn_cancel_.i18n(i18n_eval("NANA_BUTTON_CANCEL_SHORTKEY"));

			btn_cancel_.events().click.connect_unignorable([this](const arg_click&)
			{
				API::close_window(handle());
			});

			selection_.type = kind::none;
			_m_layout();
			_m_init_tree();

			if(title.empty())
			{
				const char* idstr{ nullptr };
				switch(dialog_mode)
				{
				case mode::open_file:
					idstr = "NANA_FILEBOX_OPEN";
					break;
				case mode::write_file:
					idstr = "NANA_FILEBOX_SAVE_AS";
					break;
				case mode::open_directory:
					idstr = "NANA_FILEBOX_OPEN_DIRECTORY";
					break;
				}
				this->i18n(i18n_eval(idstr));
			}
			else
				caption(title);


			if(!allow_multi_select)
				ls_file_.enable_single(true, true);
		}

		void def_extension(const std::string& ext)
		{
			def_ext_ = ext;
		}

		void load_fs(const std::string& init_path, const std::string& init_file)
		{
			//Simulate the behavior like Windows7's lpstrInitialDir(http://msdn.microsoft.com/en-us/library/windows/desktop/ms646839%28v=vs.85%29.aspx)

			//Phase 1
			std::string dir;

			auto pos = init_file.find_last_of("\\/");
			auto filename = (pos != init_file.npos ? init_file.substr(pos + 1) : init_file);

			if(saved_init_path != init_path)
			{
				if(saved_init_path.size() == 0)
					saved_init_path = init_path;

				//Phase 2: Check whether init_file contains a path
				if(filename == init_file)
				{
					//Phase 3: Check whether init_path is empty
					if(init_path.size())
						dir = init_path;
				}
				else
					dir = init_file.substr(0, pos);
			}
			else
				dir = saved_selected_path;

			_m_enter_folder(dir.size() ? dir : fs_ext::path_user().native());

			tb_file_.caption(filename);
		}

		void add_filter(const std::string& desc, const std::string& type)
		{
			std::size_t i = cb_types_.the_number_of_options();
			cb_types_.push_back(desc);
			if(0 == i)
				cb_types_.option(0);

			std::vector<std::string> v;
			std::size_t beg = 0;
			while(true)
			{
				auto pos = type.find(';', beg);
				auto ext = type.substr(beg, pos == type.npos ? type.npos : pos - beg);
				auto dot = ext.find('.');
				if((dot != ext.npos) && (dot + 1 < ext.size()))
				{
					ext.erase(0, dot + 1);
					if(ext == "*")
					{
						v.clear();
						break;
					}
					else
						v.push_back(ext);
				}
				if(pos == type.npos)
					break;
				beg = pos + 1;
			}
			if(v.size())
				cb_types_.anyobj(i, v);
		}

		std::vector<std::string> files() const
		{
			if(kind::none == selection_.type)
				return {};

			auto pos = selection_.targets.front().find_last_of("\\/");
			if(pos != selection_.targets.front().npos)
				saved_selected_path = selection_.targets.front().substr(0, pos);
			else
				saved_selected_path.clear();

			return selection_.targets;
		}
	private:
		void _m_layout()
		{
			unsigned ascent, desent, ileading;
			paint::graphics{nana::size{1, 1}}.text_metrics(ascent, desent, ileading);

			auto text_height = ascent + desent + 16;
			place_.bind(*this);
			place_.div(	"vert"
					"<weight=" + std::to_string(text_height) +" margin=5 path arrange=[variable,200] gap=5>"
					"<weight=30 margin=[0,0,5,10] new_folder arrange=[100]>"
					"<content arrange=[180] gap=[5]><weight=8>"
					"<weight=26<weight=100><vert weight=80 label margin=[0,5,0,0]>"
					"<file margin=[0,18,0,5] arrange=[variable,variable,190] gap=[10]>>"
					"<weight=48 margin=[8,0,14]<>"
					"<buttons weight=208 margin=[0,18,0] gap=[14]>>");

			place_.field("path")<<path_<<filter_;

			place_.field("new_folder")<<btn_folder_;
			place_.field("content")<<tree_<<ls_file_;
			place_.field("label")<<lb_file_;
			place_.field("file")<<tb_file_;

			if(!pick_directory_)
				place_.field("file")<<cb_types_;

			place_.field("buttons")<<btn_ok_<<btn_cancel_;
			place_.collocate();
		}

		void _m_init_tree()
		{
			//The path in linux is starting with the character '/', the name of root key should be
			//"FS.HOME", "FS.ROOT". Because a key of the tree widget should not be '/'
			nodes_.home = tree_.insert("FS.HOME", "Home");
			nodes_.home.value(kind::filesystem);
			nodes_.home.icon("icon-home");
			nodes_.filesystem = tree_.insert("FS.ROOT", "Filesystem");
			nodes_.filesystem.value(kind::filesystem);
			nodes_.filesystem.icon("icon-fs");

			std::vector<std::pair<std::string, treebox::item_proxy>> paths;
			paths.emplace_back(fs_ext::path_user().native(), nodes_.home);
			paths.emplace_back("/", nodes_.filesystem);

			fs::directory_iterator end;
			for (auto & p : paths)
			{
				for (fs::directory_iterator i{p.first}; i != end; ++i)
				{
					auto name = i->path().filename().native();
					if (!is_directory(i->status()) || (name.size() && name[0] == '.'))
						continue;

					item_proxy node = tree_.insert(p.second, name, name);
					if (!node.empty())
					{
						node.value(kind::filesystem);
						break;
					}
				}
			}

			tree_.events().expanded.connect_unignorable([this](const arg_treebox& arg)
			{
				_m_tr_expand(arg.item, arg.operated);
			});

			tree_.events().selected.connect_unignorable([this](const arg_treebox& arg)
			{
				if(arg.operated && (arg.item.value<kind::t>() == kind::filesystem))
				{
					auto path = tree_.make_key_path(arg.item, "/") + "/";
					_m_resolute_path(path);
					_m_enter_folder(path);
				}
			});
		}

		std::string _m_resolute_path(std::string& path)
		{
			auto pos = path.find('/');
			if(pos != path.npos)
			{
				auto begstr = path.substr(0, pos);
				if(begstr == "FS.HOME")
					path.replace(0, 7, fs_ext::path_user().native());
				else
					path.erase(0, pos);
				return begstr;
			}
			return std::string();
		}

		void _m_load_path(const std::string& path)
		{
			addr_.filesystem = path;
			if(addr_.filesystem.size() && addr_.filesystem[addr_.filesystem.size() - 1] != '/')
				addr_.filesystem += '/';

			file_container_.clear();

			fs::directory_iterator end;
			for(fs::directory_iterator i(path); i != end; ++i)
			{
				auto name = i->path().filename().native();
				if(name.empty() || (name.front() == '.'))
					continue;

				auto fpath = i->path().native();
				auto fattr = fs::status(fpath);
				auto ftype = static_cast<fs::file_type>(fattr.type());

				item_fs m;
				m.name = name;
				m.directory = fs::is_directory(fattr);

				if (ftype == fs::file_type::not_found ||
					ftype == fs::file_type::unknown ||
					ftype == fs::file_type::directory)
					m.bytes = 0;
				else
					m.bytes = fs::file_size(fpath);

				fs_ext::modified_file_time(fpath, m.modified_time);

				file_container_.push_back(m);

				if(m.directory)
					path_.childset(m.name, 0);
			}
			std::sort(file_container_.begin(), file_container_.end(), pred_sort_fs());
		}

		void _m_enter_folder(std::string path)
		{
			if((path.size() == 0) || (path[path.size() - 1] != '/'))
				path += '/';

			auto beg_node = tree_.selected();
			while(!beg_node.empty() && (beg_node != nodes_.home) && (beg_node != nodes_.filesystem))
				beg_node = beg_node.owner();

			auto head = fs_ext::path_user().native();
			if(path.size() >= head.size() && (path.substr(0, head.size()) == head))
			{//This is HOME
				path_.caption("HOME");
				if(beg_node != nodes_.home)
					nodes_.home->select(true);
			}
			else
			{	//Redirect to '/'
				path_.caption("FILESYSTEM");
				if(beg_node != nodes_.filesystem)
					nodes_.filesystem->select(true);
				head.clear();
			}

			if(head.size() == 0 || head[head.size() - 1] != '/')
				head += '/';


			fs::directory_iterator end;
			for(fs::directory_iterator i(head); i != end; ++i)
			{
				if(is_directory(*i))
					path_.childset(i->path().filename().native(), 0);
			}
			auto cat_path = path_.caption();
			if(cat_path.size() && cat_path[cat_path.size() - 1] != '/')
				cat_path += '/';


			auto beg = head.size();
			while(true)
			{
				auto pos = path.find('/', beg);
				auto folder = path.substr(beg, pos != path.npos ? pos - beg: path.npos);

				if(folder.empty())
					break;

				(cat_path += folder) += '/';
				(head += folder) += '/';
				path_.caption(cat_path);

				try
				{
					for(fs::directory_iterator i(head); i != end; ++i)
					{
						if (is_directory(*i))
							path_.childset(i->path().filename().native(), 0);
					}
				}
				catch(fs::filesystem_error&)
				{
					//The directory iterator may throw filesystem_error when
					//the user doesn't have permission to access the directory.

					//It just loads the sub-directories
					//to the category path.
				}

				if(pos == path.npos)
					break;
				beg = pos + 1;
			}

			try
			{
				_m_load_path(path);
				_m_list_fs();
			}
			catch(fs::filesystem_error&)
			{
				file_container_.clear();

				drawing dw{ls_file_};
				dw.clear();
				dw.draw([](paint::graphics& graph){
					std::string text = "Permission denied to access the directory";
					auto txt_sz = graph.text_extent_size(text);
					auto sz = graph.size();

					graph.string({static_cast<int>(sz.width - txt_sz.width) / 2, static_cast<int>(sz.height - txt_sz.height) / 2}, text, colors::dark_gray);
				});

				ls_file_.clear();
			}
		}

		bool _m_filter_allowed(const std::string& name, bool is_dir, const std::string& filter, const std::vector<std::string>* extension) const
		{
			if(filter.size() && (name.find(filter) == filter.npos))
				return false;

			if(pick_directory_)
				return is_dir;

			if(is_dir || (nullptr == extension) || extension->empty()) return true;

			for(auto & extstr : *extension)
			{
				auto pos = name.rfind(extstr);
				if((pos != name.npos) && (name.size() == pos + extstr.size()))
					return true;
			}
			return false;
		}

		void _m_list_fs()
		{
			drawing{ls_file_}.clear();

			auto filter = filter_.caption();
			ls_file_.auto_draw(false);

			ls_file_.clear();

			auto ext_types = cb_types_.anyobj<std::vector<std::string> >(cb_types_.option());
			auto cat = ls_file_.at(0);
			for(auto & fs: file_container_)
			{
				if(_m_filter_allowed(fs.name, fs.directory, filter, ext_types))
				{
					auto m = cat.append(fs);
					m.value(fs);

					if(fs.directory)
						m.icon(images_.folder);
					else
					{
						std::string filename = fs.name;
						for(auto ch : fs.name)
						{
							if('A' <= ch && ch <= 'Z')
								ch = ch - 'A' + 'a';

							filename += ch;
						}

						auto size = filename.size();
						paint::image use_image;

						if(size > 3)
						{
							auto ext3 = filename.substr(size - 3);
							if((".7z" == ext3) || (".ar" == ext3) || (".gz" == ext3) || (".xz" == ext3))
								use_image = images_.package;
						}

						if(use_image.empty() && (size > 4))
						{
							auto ext4 = filename.substr(size - 4);

							if( (".exe" == ext4) ||
								(".dll" == ext4))
								use_image = images_.exec;
							else if((".zip" == ext4) || (".rar" == ext4) ||
									(".bz2" == ext4) || (".tar" == ext4))
								use_image = images_.package;
							else if(".txt" == ext4)
								use_image = images_.text;
							else if ((".xml" == ext4) || (".htm" == ext4))
								use_image = images_.xml;
							else if((".jpg" == ext4) ||
									(".png" == ext4) ||
									(".gif" == ext4) ||
									(".bmp" == ext4))
								use_image = images_.image;
							else if(".pdf" == ext4)
								use_image = images_.pdf;
						}

						if(use_image.empty() && (size > 5))
						{
							auto ext5 = filename.substr(size - 5);
							if(".lzma" == ext5)
								use_image = images_.package;
							else if(".html" == ext5)
								use_image = images_.xml;
						}

						if(use_image.empty())
							m.icon(images_.file);
						else
							m.icon(use_image);

					}
				}
			}
			ls_file_.auto_draw(true);
		}

		void _m_finish(kind::t type)
		{
			selection_.type = type;
			close();
		}

		struct folder_creator
		{
			folder_creator(filebox_implement& fb, form & fm, textbox& tx)
				:	fb_(fb), fm_(fm), tx_path_(tx)
			{}

			void operator()()
			{
				auto path = tx_path_.caption();

				internationalization i18n;
				msgbox mb(fm_, i18n("NANA_FILEBOX_NEW_FOLDER"));
				mb.icon(msgbox::icon_warning);
				if(0 == path.size() || path[0] == '.' || path[0] == '/')
				{
					mb << i18n("NANA_FILEBOX_ERROR_INVALID_FOLDER_NAME");
					mb();
					return;
				}

				fs::path fspath(fb_.addr_.filesystem + path);

				auto fattr = fs::status(fspath);
				auto ftype = static_cast<fs::file_type>(fattr.type());

				if(ftype != fs::file_type::not_found && ftype != fs::file_type::none)
				{
					mb<<i18n("NANA_FILEBOX_ERROR_RENAME_FOLDER_BECAUSE_OF_EXISTING");
					mb();
					return;
				}

				if(false == fs::create_directory(fspath))
				{
					mb<<i18n("NANA_FILEBOX_ERROR_RENAME_FOLDER_BECAUSE_OF_FAILED_CREATION");
					mb();
					return;
				}

				fb_._m_enter_folder(fb_.addr_.filesystem);
				fm_.close();
			}

			filebox_implement& fb_;
			form& fm_;
			textbox & tx_path_;
		};

		bool _m_append_def_extension(std::string& tar) const
		{
			auto dotpos = tar.find_last_of('.');
			if(dotpos != tar.npos)
			{
				auto pos = tar.find_last_of("/\\");
				if(pos == tar.npos || pos < dotpos)
					return false;
			}

			if(def_ext_.size())
			{
				if(def_ext_[0] != '.')
					tar += '.';
				tar += def_ext_;
			}

			auto exts = cb_types_.anyobj<std::vector<std::string> >(cb_types_.option());
			if(0 == exts || exts->size() == 0)	return false;

			auto & ext = exts->at(0);
			if(def_ext_[0] != '.')
				tar += '.';
			tar += ext;
			return true;
		}
	private:
		void _m_insert_filename(const std::string& name)
		{
			if(selection_.targets.cend() == std::find(selection_.targets.cbegin(), selection_.targets.cend(), name))
				selection_.targets.push_back(name);
		}

		bool _m_hovered_good() const
		{
			auto pos = ls_file_.hovered(false);
			if(!pos.empty())
			{
				item_fs mfs;
				ls_file_.at(pos).resolve_to(mfs);
				return !mfs.directory;
			}
			return false;
		}

		bool _m_has_good_select() const
		{
			auto selected_items = ls_file_.selected();
			if(selected_items.size())
			{
				for(auto & pos : selected_items)
				{
					item_fs mfs;
					ls_file_.at(pos).resolve_to(mfs);

					if((mode::open_directory == mode_) || (false == mfs.directory))
						return true;
				}
			}
			return false;
		}

		bool _m_sync_with_selection()
		{
			if(_m_has_good_select())
			{
				auto selected_items = ls_file_.selected();
				if(selected_items.size() && (selected_items.size() < selection_.targets.size()))
				{
					selection_.targets.clear();
					for(auto & pos : selected_items)
					{
						item_fs mfs;
						ls_file_.at(pos).resolve_to(mfs);

						if((mode::open_directory == mode_) || (false == mfs.directory))
							selection_.targets.push_back(mfs.name);
					}
					return true;
				}
			}
			return false;
		}

		//pos must be valid
		item_fs _m_item_fs(const listbox::index_pair& pos) const
		{
			item_fs m;
			ls_file_.at(pos).resolve_to(m);
			return m;
		}

		void _m_list_dbl_clicked()
		{
			auto selected_item = ls_file_.hovered(false);
			if(selected_item.empty())
				return;

			item_fs m = _m_item_fs(selected_item);
			if(m.directory)
			{
				_m_enter_folder(addr_.filesystem + m.name + "/");
				allow_fall_back_ = true;
			}
			else
				_m_try_select(m.name);
		}

		void _m_select_file(const listbox::item_proxy& item)
		{
			item_fs mfs;
			ls_file_.at(item.pos()).resolve_to(mfs);

			if(item.selected())
			{
				if((mode::open_directory == mode_) || (false == mfs.directory))
				{
					if(ls_file_.selected().size() < 2)
						selection_.targets.clear();

					_m_insert_filename(mfs.name);
				}
			}
			else if(_m_hovered_good() || _m_has_good_select())
			{
				for(auto i = selection_.targets.cbegin(); i != selection_.targets.cend();)
				{
					std::filesystem::path p{*i};
					if(p.filename().u8string() == mfs.name)
					{
						if(!selection_.is_deselect_delayed)
						{
							i = selection_.targets.erase(i);
							continue;
						}
					}
					++i;
				}
			}
			_m_display_target_filenames();
		}

		void _m_display_target_filenames()
		{
			std::string filename_string;
			if(selection_.targets.size() == 1)
			{
				filename_string = selection_.targets.front();
			}
			else
			{
				for(auto i = selection_.targets.crbegin(); i != selection_.targets.crend(); ++i)
				{
					std::filesystem::path p{*i};
					if(!filename_string.empty())
						filename_string += ' ';

					filename_string += "\"" + p.filename().u8string() + "\"";
				}
			}

			tb_file_.caption(filename_string);
		}

		std::vector<std::string> _m_strip_files(const std::string& text)
		{
			std::vector<std::string> files;
			std::size_t start_pos = 0;
			while(true)
			{
				while(true)
				{
					auto pos = text.find_first_of(" \"", start_pos);
					if(text.npos == pos)
					{
						if(text.length() == start_pos)
							return files;
						else if(0 == start_pos) //single selection
							return {text};

						return {};
					}

					start_pos = pos + 1;

					if(text[pos] == '"')
						break;
				}

				auto pos = text.find('"', start_pos);
				if(text.npos == pos)
					return {};

				if(pos - start_pos > 0)
					files.push_back(text.substr(start_pos, pos - start_pos));

				start_pos = pos + 1;
			}

			return files;
		}

		void _m_msgbox(const char* i18n_idstr, const std::string& arg) const
		{
			internationalization i18n;

			msgbox mb(*this, caption());
			mb.icon(msgbox::icon_warning);
			mb<<i18n(i18n_idstr, arg);
			mb();
		}


		void _m_try_select(const std::string& files_text)
		{
			selection_.targets.clear();
			auto targets = _m_strip_files(files_text);

			if(targets.empty())
			{
				if(mode::open_directory != mode_)
					return;

				targets.push_back(files_text);
			}

			internationalization i18n;

			//the position of tar in targets
			int tar_idx = -1;

			//Check whether the selected files are valid.
			for(auto tar : targets)
			{
				++tar_idx;
				if(tar[0] == '.')
				{
					_m_msgbox("NANA_FILEBOX_ERROR_INVALID_FILENAME", tar);
					return;
				}

				if(tar[0] != '/')
					tar = addr_.filesystem + tar;

				auto fattr = fs::status(tar);
				auto ftype = static_cast<fs::file_type>(fattr.type());

				//Check if the selected name is a directory
				auto is_dir = fs::is_directory(fattr);

				if(!is_dir && _m_append_def_extension(tar))
				{
					//Add the extension, then check if it is a directory again.
					fattr = fs::status(tar);
					ftype = static_cast<fs::file_type>(fattr.type());
					is_dir = fs::is_directory(fattr);
				}

				if(mode::open_directory == mode_)
				{
					//In open_directory mode, all the folders must be valid
					if(!is_dir)
					{
						fs::path p{tar};
						auto p1 = p.filename();
						auto p2 = p.parent_path().filename();
						if(allow_fall_back_ && (p.filename() == p.parent_path().filename()))
						{
							//fallback check, and redirects to its parent path.
							tar = p.parent_path().string();
						}
						else
						{

							const char* i18n_idstr = "NANA_FILEBOX_ERROR_DIRECTORY_INVALID";
							if(fs::file_type::not_found == ftype)
								i18n_idstr = "NANA_FILEBOX_ERROR_DIRECTORY_NOT_EXISTING_AND_RETRY";

							_m_msgbox(i18n_idstr, p.filename().string());
							return;
						}
					}
				}
				else
				{
					//Enter the directory if this is the first tar.
					if(is_dir)
					{
						if(0 == tar_idx)
						{
							_m_enter_folder(tar);
							tb_file_.caption(std::string{});
							return;
						}
						//Other folders are ignored
						continue;
					}

					if(mode::write_file != mode_)
					{
						if(fs::file_type::not_found == ftype)
						{
							msgbox mb(*this, caption());
							mb.icon(msgbox::icon_information);
							if(mode::open_file == mode_)
								mb << i18n("NANA_FILEBOX_ERROR_NOT_EXISTING_AND_RETRY", tar);
							else
								mb << i18n("NANA_FILEBOX_ERROR_DIRECTORY_NOT_EXISTING_AND_RETRY", tar);
							mb();

							return;
						}
					}
					else
					{
						if(fs::file_type::not_found != ftype)
						{
							msgbox mb(*this, caption(), msgbox::yes_no);
							mb.icon(msgbox::icon_question);
							mb<<i18n("NANA_FILEBOX_ERROR_QUERY_REWRITE_BECAUSE_OF_EXISTING");
							if(msgbox::pick_no == mb())
								return;
						}
					}
				}

				auto pos = tar.find_last_not_of("\\/");
				if(pos != tar.npos)
					tar.erase(pos + 1);

				selection_.targets.push_back(tar);
			}

			_m_finish(kind::filesystem);
		}

		void _m_tr_expand(item_proxy node, bool exp)
		{
			if(false == exp) return;

			if(kind::filesystem == node.value<kind::t>())
			{
				auto path = tree_.make_key_path(node, "/") + "/";
				_m_resolute_path(path);

				fs::directory_iterator end;
				for (fs::directory_iterator i{path}; i != end; ++i)
				{
					auto name = i->path().filename().native();
					if((!is_directory(*i)) || (name.size() && name[0] == '.'))
						continue;

					auto child = node.append(name, name, kind::filesystem);
					if(!child.empty())
					{
						child->icon("icon-folder");
						//The try-catch can be eliminated by using
						//directory_iterator( const std::filesystem::path& p, std::error_code& ec ) noexcept;
						//in C++17
						try
						{
							for(fs::directory_iterator u(i->path()); u != end; ++u)
							{
								auto uname = u->path().filename().native();
								if ((!is_directory(*u)) || (uname.size() && uname[0] == '.'))
									continue;

								child.append(uname, uname, kind::filesystem);
								break;
							}
						}
						catch(fs::filesystem_error&)
						{
							//The directory iterator may throw filesystem_error when
							//the user doesn't have permission to access the directory.

							//Catch the error without any process, because the loop is just
							//to peak whether the directory(i->path) has a sub-directory.
						}
					}
				}
			}
		}
	private:
		bool const pick_directory_;
		mode mode_;
		std::string def_ext_;

		place	place_;
		categorize<int> path_;
		textbox		filter_;
		button	btn_folder_;
		treebox tree_;
		listbox	ls_file_;

		label lb_file_;
		textbox	tb_file_;
		combox	cb_types_;
		button btn_ok_, btn_cancel_;
		bool allow_fall_back_{false};

		struct tree_node_tag
		{
			item_proxy home;
			item_proxy filesystem;
		}nodes_;

		std::vector<item_fs> file_container_;
		struct path_rep
		{
			std::string filesystem;
		}addr_;

		struct selection_rep
		{
			kind::t type;
			std::vector<std::string> targets;
			bool is_deselect_delayed{ false };
		}selection_;

		static std::string saved_init_path;
		static std::string saved_selected_path;
		nana::detail::theme theme_;

		struct images
		{
			paint::image folder;
			paint::image file;
			paint::image exec;
			paint::image package;
			paint::image text;
			paint::image xml;
			paint::image image;
			paint::image pdf;
		}images_;
	};//end class filebox_implement
	std::string filebox_implement::saved_init_path;
	std::string filebox_implement::saved_selected_path;

#endif
	//class filebox
	struct filebox::implement
	{
		struct filter
		{
			std::string des;
			std::string type;
		};

		window owner;
		bool open_or_save;

		bool allow_multi_select;
		std::string init_file;

		std::string title;
		path_type path;
		std::vector<filter> filters;
	};

	filebox::filebox(window owner, bool open)
		: impl_(new implement)
	{
		impl_->owner = owner;
		impl_->open_or_save = open;
		impl_->allow_multi_select = false;
#if defined(NANA_WINDOWS)
		auto len = ::GetCurrentDirectory(0, nullptr);
		if(len)
		{
			std::wstring path;
			path.resize(len + 1);
			::GetCurrentDirectory(len, &(path[0]));
			path.resize(len);

			impl_->path = to_utf8(path);
        }
#endif
	}

	filebox::filebox(const filebox& other)
		: impl_(new implement(*other.impl_))
	{}

	filebox::~filebox()
	{
		delete impl_;
	}

	filebox& filebox::operator=(const filebox& other)
	{
		if (this != &other)
			*impl_ = *other.impl_;
		return *this;
	}

	void filebox::owner(window wd)
	{
		impl_->owner = wd;
	}

	filebox& filebox::title(std::string s)
	{
		impl_->title.swap(s);
		return *this;
	}

	filebox& filebox::init_path(const path_type& p)
	{
		std::error_code err;
		if (p.empty() || is_directory(p, err))
			impl_->path = p;
		
		return *this;
	}

	filebox& filebox::init_file(const std::string& ifstr)
	{
		impl_->init_file = ifstr;
		return *this;
	}

	filebox& filebox::add_filter(const std::string& description, const std::string& filetype)
	{
		implement::filter flt = {description, filetype};
		impl_->filters.push_back(flt);
		return *this;
	}

	filebox& filebox::add_filter(const std::vector<std::pair<std::string, std::string>> &filters)
	{
		for (auto &f : filters)
			add_filter(f.first, f.second);
		return *this;
	}

	const filebox::path_type& filebox::path() const
	{
		return impl_->path;
	}

	std::vector<filebox::path_type> filebox::show() const
	{
		std::vector<path_type> targets;

#if defined(NANA_WINDOWS)
		std::wstring wfile = to_wstring(impl_->init_file);
		wfile.resize(impl_->allow_multi_select ? (520 + 32*256) : 520);

		OPENFILENAME ofn;
		memset(&ofn, 0, sizeof ofn);

		internal_scope_guard lock;

		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = reinterpret_cast<HWND>(API::root(impl_->owner));
		ofn.lpstrFile = &(wfile[0]);
		ofn.nMaxFile = static_cast<DWORD>(wfile.size() - 1);

		const wchar_t * filter;
		std::wstring filter_holder;
		std::wstring default_extension;
		if(impl_->filters.size())
		{
			for(auto & f : impl_->filters)
			{
				filter_holder += to_wstring(f.des);
				filter_holder += static_cast<std::wstring::value_type>('\0'); // separator
				std::wstring ff = to_wstring(f.type);
				std::size_t pos = 0;
				while(true)   // eliminate spaces
				{
					pos = ff.find(L" ", pos);
					if(pos == ff.npos)
						break;
					ff.erase(pos);
				}
				filter_holder += ff;
				filter_holder += static_cast<std::wstring::value_type>('\0'); // separator

				//Get the default file extension
				if (default_extension.empty())
				{
					pos = ff.find_last_of('.');
					if (pos != ff.npos)
					{
						ff = ff.substr(pos + 1);
						if (ff != L"*")
						{
							default_extension = ff;
							ofn.lpstrDefExt = default_extension.data();
						}
					}
				}
			}
			filter = filter_holder.data();
		}
		else
			filter = L"All Files\0*.*\0";

		auto wtitle = to_wstring(impl_->title);
		auto wpath = to_wstring(impl_->path);
		ofn.lpstrFilter = filter;
		ofn.lpstrTitle = (wtitle.empty() ? nullptr : wtitle.c_str());
		ofn.nFilterIndex = 0;
		ofn.lpstrFileTitle = nullptr;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = (wpath.empty() ? nullptr : wpath.c_str());

		if (!impl_->open_or_save)
			ofn.Flags = OFN_OVERWRITEPROMPT;	//Overwrite prompt if it is save mode
		else ofn.Flags = OFN_FILEMUSTEXIST;	//In open mode, user can't type name of nonexistent file
		ofn.Flags |= OFN_NOCHANGEDIR;
		if(impl_->allow_multi_select)
		{
			ofn.Flags |= (OFN_ALLOWMULTISELECT | OFN_EXPLORER);
		}

		{
			internal_revert_guard revert;
			if (FALSE == (impl_->open_or_save ? ::GetOpenFileName(&ofn) : ::GetSaveFileName(&ofn)))
				return targets;
		}

		if(impl_->allow_multi_select)
		{
			const wchar_t* str = ofn.lpstrFile;
			auto len = ::wcslen(str);

			path_type parent_path{ str };
			str += (len + 1);

			while(*str)
			{
				len = ::wcslen(str);
				targets.emplace_back(parent_path / path_type{str});
				str += (len + 1);
			}
			impl_->path = parent_path.u8string();
		}
		else
		{
			wfile.resize(std::wcslen(wfile.data()));

			targets.emplace_back(wfile);
			impl_->path = targets.front().parent_path().u8string();
		}

#elif defined(NANA_POSIX)
		using mode = filebox_implement::mode;
		filebox_implement fb(impl_->owner, (impl_->open_or_save ? mode::open_file : mode::write_file), impl_->title, false, impl_->allow_multi_select);

		if(impl_->filters.size())
		{
			for(auto & f: impl_->filters)
			{
				std::string fs = f.type;
				std::size_t pos = 0;
				while(true)
				{
					pos = fs.find(" ", pos);
					if(pos == fs.npos)
						break;
					fs.erase(pos);
				}
				fb.add_filter(f.des, fs);
			}
		}
		else
			fb.add_filter("All Files", "*.*");

		fb.load_fs(impl_->path, impl_->init_file);

		API::modal_window(fb);


		for(auto & f : fb.files())
			targets.emplace_back(f);


		if(!targets.empty())
			impl_->path = targets.front().parent_path().u8string();
		else
			impl_->path.clear();
#endif
		return targets;
	}


	filebox& filebox::allow_multi_select(bool allow)
	{
		impl_->allow_multi_select = allow;
		return *this;
	}
	//end class filebox

	//class directory picker
	struct folderbox::implement
	{
		window owner;
		path_type init_path;
		std::string title;
		bool allow_multi_select;
	};

	folderbox::folderbox(window owner, const path_type& init_path, std::string title)
		: impl_(new implement{ owner, fs::weakly_canonical(init_path).make_preferred(), title, false})
	{}


	folderbox::~folderbox()
	{
		delete impl_;
	}


	folderbox& folderbox::title(std::string s)
	{
		impl_->title.swap(s);
		return *this;
	}


#ifdef NANA_MINGW
	static int CALLBACK browse_folder_callback(HWND hwnd, UINT msg, LPARAM lparam, LPARAM data)
	{
		// If the BFFM_INITIALIZED message is received
		// set the path to the start path.
		switch (msg)
		{
		case BFFM_INITIALIZED:
			if (data)
			{
				SendMessage(hwnd, BFFM_SETSELECTION, TRUE, data);
			}
			break;
		}

		return 0; // The function should always return 0.
	}
#endif

	folderbox& folderbox::allow_multi_select(bool allow)
	{
		impl_->allow_multi_select = allow;
		return *this;
	}

	std::vector<folderbox::path_type> folderbox::show() const
	{
		std::vector<path_type> targets;
#ifdef NANA_WINDOWS

		nana::detail::platform_spec::co_initializer co_init;
#ifndef NANA_MINGW
		IFileOpenDialog *fd(nullptr);
		HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&fd));
		if (SUCCEEDED(hr))
		{
			IShellItem *init_path{ nullptr };
			hr = SHCreateItemFromParsingName(impl_->init_path.wstring().c_str(), nullptr, IID_PPV_ARGS(&init_path));
			if (SUCCEEDED(hr))
				fd->SetFolder(init_path);

			fd->SetOptions(FOS_PICKFOLDERS | (impl_->allow_multi_select ? FOS_ALLOWMULTISELECT : 0));
			fd->Show(reinterpret_cast<HWND>(API::root(impl_->owner))); // the native handle of the parent nana form goes here

			::IShellItemArray *sia;
			if (SUCCEEDED(fd->GetResults(&sia))) // fails if user cancelled
			{
				DWORD num_items;
				if (SUCCEEDED(sia->GetCount(&num_items)))
				{
					for (DWORD i = 0; i < num_items; ++i)
					{
						::IShellItem* si;
						if (SUCCEEDED(sia->GetItemAt(i, &si)))
						{
							PWSTR pwstr(nullptr);
							if (SUCCEEDED(si->GetDisplayName(SIGDN_FILESYSPATH, &pwstr)))
							{
								targets.emplace_back(pwstr);
								// use the c-string pointed to by pwstr here
								::CoTaskMemFree(pwstr);
							}
							si->Release();
						}
					}
				}
				sia->Release();
			}
			fd->Release();
		}
#else
		wchar_t	display_text[MAX_PATH];
		auto title = to_wstring( impl_->title ) ;
		std::wstring init_path = impl_->init_path.wstring();

		// https://docs.microsoft.com/en-us/windows/desktop/api/shlobj_core/ns-shlobj_core-_browseinfoa
		BROWSEINFO brw       = { 0 };
		brw.hwndOwner        = reinterpret_cast<HWND>(API::root(impl_->owner));
		brw.pszDisplayName   = display_text; // buffer to receive the display name of the folder selected by the user.
		brw.lpszTitle        = title.data();
		brw.ulFlags          = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE; // | BIF_EDITBOX;
		brw.lpfn             = browse_folder_callback;
		brw.lParam           = reinterpret_cast<LPARAM>(init_path.c_str());

		auto pidl = ::SHBrowseForFolder(&brw);
		if (pidl)
		{
			wchar_t folder_path[MAX_PATH];
			if (FALSE != SHGetPathFromIDList(pidl, folder_path))
				targets.emplace_back(folder_path);

			co_init.task_mem_free(pidl);
		}
#endif

#elif defined(NANA_POSIX)
		using mode = filebox_implement::mode;
		filebox_implement fb(impl_->owner, mode::open_directory, {}, true, impl_->allow_multi_select);

		fb.load_fs(impl_->init_path, "");

		API::modal_window(fb);

		auto path_dirs = fb.files();

		for(auto & p: path_dirs)
			targets.push_back(p);
#endif
		return targets;
	}
}//end namespace nana
