/*
*	Filebox
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/filebox.cpp
*/

#include <nana/gui.hpp>
#include <nana/gui/filebox.hpp>
#include <nana/filesystem/filesystem.hpp>

#if defined(NANA_WINDOWS)
	#include <windows.h>
#elif defined(NANA_POSIX)
	#include <nana/gui/widgets/label.hpp>
	#include <nana/gui/widgets/button.hpp>
	#include <nana/gui/widgets/listbox.hpp>
	#include <nana/gui/widgets/categorize.hpp>
	#include <nana/gui/widgets/textbox.hpp>
	#include <nana/gui/widgets/treebox.hpp>
	#include <nana/gui/widgets/combox.hpp>
	#include <nana/filesystem/filesystem.hpp>
	#include <nana/gui/place.hpp>
	#include <stdexcept>
	#include <algorithm>
#endif

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
			nana::long_long_t bytes;

			friend listbox::iresolver& operator>>(listbox::iresolver& ires, item_fs& m)
			{
				std::wstring type;
				ires>>m.name>>type>>type;
				m.directory = (type == L"Directory");
				return ires;
			}

			friend listbox::oresolver& operator<<(listbox::oresolver& ores, const item_fs& item)
			{
				std::wstringstream tm;
				tm<<(item.modified_time.tm_year + 1900)<<'-';
				_m_add(tm, item.modified_time.tm_mon + 1)<<'-';
				_m_add(tm, item.modified_time.tm_mday)<<' ';

				_m_add(tm, item.modified_time.tm_hour)<<':';
				_m_add(tm, item.modified_time.tm_min)<<':';
				_m_add(tm, item.modified_time.tm_sec);

				ores<<item.name<<tm.str();
				if(!item.directory)
				{
					auto pos = item.name.find_last_of('.');
					if(pos != item.name.npos && (pos + 1 < item.name.size()))
						ores<<item.name.substr(pos + 1);
					else
						ores<<L"File";

					ores<<_m_trans(item.bytes);
				}
				else
					ores<<L"Directory";
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

			static std::string _m_trans(std::size_t bytes)
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
				ss<<bytes<<" Bytes";
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
		struct kind
		{
			enum t{none, filesystem};
		};

		typedef treebox::item_proxy item_proxy;
	public:

		filebox_implement(window owner, bool io_read, const std::string& title)
			: form(owner, API::make_center(owner, 630, 440)), io_read_(io_read)
		{
			path_.create(*this);
			path_.splitstr("/");
			path_.events().selected.connect_unignorable([this](const arg_categorize<int>&)
			{
				auto path = path_.caption();
				auto root = path.substr(0, path.find('/'));
				if(root == "HOME")
					path.replace(0, 4, nana::filesystem::path_user().native());
				else if(root == "FILESYSTEM")
					path.erase(0, 10);
				else
					throw std::runtime_error("Nana.GUI.Filebox: Wrong categorize path");

				if(path.size() == 0) path = "/";
				_m_load_cat_path(path);
			});

			filter_.create(*this);
			filter_.multi_lines(false);
			filter_.tip_string("Filter");

			filter_.events().key_release.connect_unignorable([this](const arg_keyboard&)
			{
				_m_list_fs();
			});

			btn_folder_.create(*this);
			btn_folder_.caption("&New Folder");

			btn_folder_.events().click.connect_unignorable([this](const arg_click&)
			{
				form fm(this->handle(), API::make_center(*this, 300, 35));
				fm.caption("Name the new folder");

				textbox folder(fm, nana::rectangle(5, 5, 160, 25));
				folder.multi_lines(false);

				button btn(fm, nana::rectangle(170, 5, 60, 25));
				btn.caption("Create");

				btn.events().click.connect_unignorable(folder_creator(*this, fm, folder));

				button btn_cancel(fm, nana::rectangle(235, 5, 60, 25));
				btn_cancel.caption("Cancel");

				btn_cancel.events().click.connect_unignorable([&fm](const arg_click&)
				{
					fm.close();
				});
				API::modal_window(fm);
			});

			tree_.create(*this);

			ls_file_.create(*this);
			ls_file_.append_header("Name", 190);
			ls_file_.append_header("Modified", 145);
			ls_file_.append_header("Type", 80);
			ls_file_.append_header("Size", 70);

			auto fn_sel_file = [this](const arg_mouse& arg){
				_m_sel_file(arg);
			};
			ls_file_.events().dbl_click.connect_unignorable(fn_sel_file);
			ls_file_.events().mouse_down.connect_unignorable(fn_sel_file);
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
			ls_file_.set_sort_compare(3, [this](const std::string&, nana::any* anyptr_a, const std::string&, nana::any* anyptr_b, bool reverse) -> bool
				{
					item_fs * fsa = any_cast<item_fs>(anyptr_a);
					item_fs * fsb = any_cast<item_fs>(anyptr_b);
					return (reverse ? fsa->bytes > fsb->bytes : fsa->bytes < fsb->bytes);
				});

			lb_file_.create(*this);
			lb_file_.caption("File:");
			
			tb_file_.create(*this);
			tb_file_.multi_lines(false);

			tb_file_.events().key_char.connect_unignorable([this](const arg_keyboard& arg)
			{
				if(arg.key == nana::keyboard::enter)
					_m_ok();
			});

			cb_types_.create(*this);
			cb_types_.editable(false);
			cb_types_.events().selected.connect_unignorable([this](const arg_combox&){ _m_list_fs(); });

			btn_ok_.create(*this);
			btn_ok_.caption("&OK");

			btn_ok_.events().click.connect_unignorable([this](const arg_click&)
			{
				_m_ok();
			});
			btn_cancel_.create(*this);
			btn_cancel_.caption("&Cancel");

			btn_cancel_.events().click.connect_unignorable([this](const arg_click&)
			{
				API::close_window(handle());
			});

			selection_.type = kind::none;
			_m_layout();
			_m_init_tree();

			if(0 == title.size())
				caption(io_read ? "Open" : "Save As");
			else
				caption(title);
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
			auto file_with_path_removed = (pos != init_file.npos ? init_file.substr(pos + 1) : init_file);

			if(saved_init_path != init_path)
			{
				if(saved_init_path.size() == 0)
					saved_init_path = init_path;

				//Phase 2: Check whether init_file contains a path
				if(file_with_path_removed == init_file)
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

			_m_load_cat_path(dir.size() ? dir : nana::filesystem::path_user().native());

			tb_file_.caption(file_with_path_removed);
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

		bool file(std::string& fs) const
		{
			if(selection_.type == kind::none)
				return false;

			auto pos = selection_.target.find_last_of("\\/");
			if(pos != selection_.target.npos)
				saved_selected_path = selection_.target.substr(0, pos);
			else
				saved_selected_path.clear();

			fs = selection_.target;
			return true;
		}
	private:
		void _m_layout()
		{
			place_.bind(*this);
			place_.div(	"vert"
					"<weight=34 margin=5 path arrange=[variable,200] gap=5>"
					"<weight=30 margin=[0,0,5,10] new_folder arrange=[100]>"
					"<content arrange=[180] gap=[5]><weight=8>"
					"<weight=26<weight=100><vert weight=60 label margin=[0,0,0,5]>"
					"<file margin=[0,18,0,5] arrange=[variable,variable,190] gap=[10]>>"
					"<weight=48 margin=[8,0,14]<>"
					"<buttons weight=208 margin=[0,18,0] gap=[14]>>");

			place_.field("path")<<path_<<filter_;
			place_.field("new_folder")<<btn_folder_;
			place_.field("content")<<tree_<<ls_file_;
			place_.field("label")<<lb_file_;
			place_.field("file")<<tb_file_<<cb_types_;
			place_.field("buttons")<<btn_ok_<<btn_cancel_;
			place_.collocate();
		}

		void _m_init_tree()
		{
			//The path in linux is starting with the character '/', the name of root key should be
			//"FS.HOME", "FS.ROOT". Because a key of the tree widget should not be '/'
			nodes_.home = tree_.insert("FS.HOME", "Home");
			nodes_.home.value(kind::filesystem);
			nodes_.filesystem = tree_.insert("FS.ROOT", "Filesystem");
			nodes_.filesystem.value(kind::filesystem);

			namespace fs = ::nana::experimental::filesystem;

			std::vector<std::string> paths;
			paths.emplace_back(fs::path_user().native());
			paths.emplace_back("/");

			fs::directory_iterator end;
			for (auto & p : paths)
			{
				for (fs::directory_iterator i(p); i != end; ++i)
				{
					auto name = i->path().filename().native();
					if (!is_directory(i->status()) || (name.size() && name[0] == '.'))
						continue;

					item_proxy node = tree_.insert(nodes_.filesystem, name, name);
					if (false == node.empty())
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
					_m_load_cat_path(path);
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
					path.replace(0, 7, nana::filesystem::path_user().native());
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

			namespace fs = ::nana::experimental::filesystem;

			fs::directory_iterator end;
			for(fs::directory_iterator i(path); i != end; ++i)
			{
				auto name = i->path().filename().native();
				if(name.empty() || (name.front() == '.'))
					continue;

				item_fs m;
				m.name = name;

				auto fattr = fs::status(path + m.name);

				if(fattr.type() != fs::file_type::not_found && fattr.type() != fs::file_type::unknown)
				{
					m.bytes = fs::file_size(path + m.name);
					m.directory = fs::is_directory(fattr);
					::nana::filesystem::modified_file_time(path + m.name, m.modified_time);
				}
				else
				{
					m.bytes = 0;
					m.directory = fs::is_directory(*i);
					::nana::filesystem::modified_file_time(path + i->path().filename().native(), m.modified_time);				
				}

				file_container_.push_back(m);

				if(m.directory)
					path_.childset(m.name, 0);
			}
			std::sort(file_container_.begin(), file_container_.end(), pred_sort_fs());
		}

		void _m_load_cat_path(std::string path)
		{
			if((path.size() == 0) || (path[path.size() - 1] != '/'))
				path += '/';

			auto beg_node = tree_.selected();
			while(!beg_node.empty() && (beg_node != nodes_.home) && (beg_node != nodes_.filesystem))
				beg_node = beg_node.owner();
			
			auto head = nana::filesystem::path_user().native();
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

			namespace fs = ::nana::experimental::filesystem;

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
				if(folder.size() == 0) break;
				(cat_path += folder) += '/';
				(head += folder) += '/';
				path_.caption(cat_path);
				
				for(fs::directory_iterator i(head); i != end; ++i)
				{
					if (is_directory(*i))
						path_.childset(i->path().filename().native(), 0);
				}

				if(pos == path.npos)
					break;
				beg = pos + 1;
			}
			_m_load_path(path);
			_m_list_fs();
		}

		bool _m_filter_allowed(const std::string& name, bool is_dir, const std::string& filter, const std::vector<std::string>* extension) const
		{
			if(filter.size() && (name.find(filter) == filter.npos))
				return false;

			if((is_dir || 0 == extension) || (0 == extension->size())) return true;

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
			auto filter = filter_.caption();
			ls_file_.auto_draw(false);

			ls_file_.clear();

			auto ext_types = cb_types_.anyobj<std::vector<std::string> >(cb_types_.option());
			auto cat = ls_file_.at(0);
			for(auto & fs: file_container_)
			{
				if(_m_filter_allowed(fs.name, fs.directory, filter, ext_types))
				{
					cat.append(fs).value(fs);
				}
			}
			ls_file_.auto_draw(true);
		}

		void _m_finish(kind::t type, const std::string& tar)
		{
			selection_.target = tar;
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

				msgbox mb(fm_, "Create Folder");
				mb.icon(msgbox::icon_warning);
				if(0 == path.size() || path[0] == '.' || path[0] == '/')
				{
					mb<<L"Please input a valid name for the new folder.";
					mb();
					return;
				}

				using file_type = nana::experimental::filesystem::file_type;

				experimental::filesystem::path fspath(fb_.addr_.filesystem + path);

				auto fs = experimental::filesystem::status(fspath);

				if(fs.type() != file_type::not_found && fs.type() != file_type::none)
				{
					mb<<L"The folder is existing, please rename it.";
					mb();
					return;
				}

				if(false == experimental::filesystem::create_directory(fspath))
				{
					mb<<L"Failed to create the folder, please rename it.";
					mb();
					return;
				}			

				fb_._m_load_cat_path(fb_.addr_.filesystem);
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
		void _m_sel_file(const arg_mouse& arg)
		{
			auto sel = ls_file_.selected();
			if(sel.empty())
				return;
			
			auto index = sel[0];
			item_fs m;
			ls_file_.at(index).resolve_to(m);

			if(event_code::dbl_click == arg.evt_code)
			{
				if(m.directory)
					_m_load_cat_path(addr_.filesystem + m.name + "/");
				else
					_m_finish(kind::filesystem, addr_.filesystem + m.name);
			}
			else
			{
				if(false == m.directory)
				{
					selection_.target = addr_.filesystem + m.name;
					tb_file_.caption(m.name);
				}
			}
		}

		void _m_ok()
		{
			if(0 == selection_.target.size())
			{
				auto file = tb_file_.caption();
				if(file.size())
				{
					if(file[0] == L'.')
					{
						msgbox mb(*this, caption());
						mb.icon(msgbox::icon_warning);
						mb<<file<<std::endl<<L"The filename is invalid.";
						mb();
						return;
					}
					std::string tar;
					if(file[0] == '/')
						tar = file;
					else
						tar = addr_.filesystem + file;


					bool good = true;

					namespace fs = ::nana::experimental::filesystem;
					auto fattr = fs::status(tar);
					if(fattr.type() == fs::file_type::not_found)
					{
						good = (_m_append_def_extension(tar) && (fs::status(tar).type() == fs::file_type::not_found));					
					}

					if(good && fs::is_directory(fattr))
					{
						_m_load_cat_path(tar);
						tb_file_.caption("");
						return;
					}
					
					if(io_read_)
					{
						if(false == good)
						{
							msgbox mb(*this, caption());
							mb.icon(msgbox::icon_information);
							mb<<L"The file \""<<nana::charset(tar, nana::unicode::utf8)<<L"\"\n is not existing. Please check and retry.";
							mb();
						}
						else
							_m_finish(kind::filesystem, tar);
					}
					else
					{
						if(good)
						{
							msgbox mb(*this, caption(), msgbox::yes_no);
							mb.icon(msgbox::icon_question);
							mb<<L"The input file is existing, do you want to overwrite it?";
							if(msgbox::pick_no == mb())
								return;
						}
						_m_finish(kind::filesystem, tar);
					}
				}
			}
			else
				_m_finish(kind::filesystem, selection_.target);
		}

		void _m_tr_expand(item_proxy node, bool exp)
		{
			if(false == exp) return;

			if(kind::filesystem == node.value<kind::t>())
			{
				auto path = tree_.make_key_path(node, "/") + "/";
				_m_resolute_path(path);

				namespace fs = ::nana::experimental::filesystem;

				fs::directory_iterator end;
				for (fs::directory_iterator i{path}; i != end; ++i)
				{
					auto name = i->path().filename().native();
					if((!is_directory(*i)) || (name.size() && name[0] == '.'))
						continue;

					auto child = node.append(name, name, kind::filesystem);
					if(!child.empty())
					{
						for(fs::directory_iterator u(i->path()); u != end; ++u)
						{
							auto uname = i->path().filename().native();
							if ((!is_directory(*i)) || (uname.size() && uname[0] == '.'))
								continue;

							child.append(uname, uname, kind::filesystem);
							break;
						}
					}
				}
			}
		}
	private:
		bool io_read_;
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
			std::string target;
		}selection_;

		static std::string saved_init_path;
		static std::string saved_selected_path;
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

			std::string file;
			std::string title;
			std::string path;
			std::vector<filter> filters;
		};

		filebox::filebox(bool is_openmode)
			: filebox(nullptr, is_openmode)
		{
		}

		filebox::filebox(window owner, bool open)
			: impl_(new implement)
		{
			impl_->owner = owner;
			impl_->open_or_save = open;
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

		std::string filebox::title(std::string s)
		{
			impl_->title.swap(s);
			return s;
		}

		filebox& filebox::init_path(const std::string& ipstr)
		{
			if(ipstr.empty())
			{
				impl_->path.clear();
			}
			else
			{
				namespace fs = ::nana::experimental::filesystem;
				if (fs::is_directory(ipstr))
					impl_->path = ipstr;
			}
			return *this;
		}

		filebox& filebox::init_file(const std::string& ifstr)
		{
			impl_->file = ifstr;
			return *this;
		}

		filebox& filebox::add_filter(const std::string& description, const std::string& filetype)
		{
			implement::filter flt = {description, filetype};
			impl_->filters.push_back(flt);
			return *this;
		}

		std::string filebox::path() const
		{
			return impl_->path;
		}
		
		std::string filebox::file() const
		{
			return impl_->file;
		}

		bool filebox::show() const
		{
#if defined(NANA_WINDOWS)
			std::wstring wfile;
			wfile.resize(520);

			OPENFILENAME ofn;
			memset(&ofn, 0, sizeof ofn);
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
					filter_holder += static_cast<std::wstring::value_type>('\0');
					std::wstring fs = to_wstring(f.type);
					std::size_t pos = 0;
					while(true)
					{
						pos = fs.find(L" ", pos);
						if(pos == fs.npos)
							break;
						fs.erase(pos);
					}
					filter_holder += fs;
					filter_holder += static_cast<std::wstring::value_type>('\0');

					//Get the default file extentsion
					if (default_extension.empty())
					{
						pos = fs.find_last_of('.');
						if (pos != fs.npos)
						{
							fs = fs.substr(pos + 1);
							if (fs != L"*")
							{
								default_extension = fs;
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
			ofn.Flags |= OFN_NOCHANGEDIR;
			
			if(FALSE == (impl_->open_or_save ? ::GetOpenFileName(&ofn) : ::GetSaveFileName(&ofn)))
				return false;
			
			wfile.resize(std::wcslen(wfile.data()));
			impl_->file = to_utf8(wfile);
#elif defined(NANA_POSIX)
			filebox_implement fb(impl_->owner, impl_->open_or_save, impl_->title);
			
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

			fb.load_fs(impl_->path, impl_->file);

			API::modal_window(fb);
			if(false == fb.file(impl_->file))
				return false;
#endif
			auto tpos = impl_->file.find_last_of("\\/");
			if(tpos != impl_->file.npos)
				impl_->path = impl_->file.substr(0, tpos);
			else
				impl_->path.clear();

			return true;
		}//end class filebox
}//end namespace nana
