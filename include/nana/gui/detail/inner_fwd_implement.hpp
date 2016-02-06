/*
 *	Implementations of Inner Forward Declaration
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/inner_fwd_implement.hpp
 *
 */

#ifndef NANA_GUI_INNER_FWD_IMPLEMENT_HPP
#define NANA_GUI_INNER_FWD_IMPLEMENT_HPP

#include "inner_fwd.hpp"
#include "basic_window.hpp"
#include "../../paint/graphics.hpp"

#include <map>

namespace nana{
	namespace detail
	{
		class shortkey_container
		{
			struct item_type
			{
				window handle;
				std::vector<unsigned long> keys;
			};
		public:
			void clear()
			{
				keybase_.clear();
			}

			bool make(window wd, unsigned long key)
			{
				if (wd == nullptr) return false;
				if (key < 0x61) key += (0x61 - 0x41);

				for (auto & m : keybase_)
				{
					if (m.handle == wd)
					{
						m.keys.push_back(key);
						return true;
					}
				}

				item_type m;
				m.handle = wd;
				m.keys.push_back(key);
				keybase_.push_back(m);

				return true;
			}

			void umake(window wd)
			{
				if (wd == nullptr) return;

				for (auto i = keybase_.begin(), end = keybase_.end(); i != end; ++i)
				{
					if (i->handle == wd)
					{
						keybase_.erase(i);
						break;
					}
				}
			}

			std::vector<unsigned long> keys(window wd) const
			{
				std::vector<unsigned long> v;
				if (wd)
				{
					for (auto & m : keybase_)
					{
						if (m.handle == wd)
						{
							v = m.keys;
							break;
						}
					}
				}
				return v;

			}

			window find(unsigned long key) const
			{
				if (key < 0x61) key += (0x61 - 0x41);

				for (auto & m : keybase_)
				{
					for (auto n : m.keys)
					{
						if (key == n)
							return m.handle;
					}
				}
				return nullptr;
			}
		private:
			std::vector<item_type> keybase_;
		};


		struct root_misc
		{
			typedef basic_window core_window_t;

			core_window_t * window;
			nana::paint::graphics	root_graph;
			shortkey_container		shortkeys;

			struct condition_tag
			{
				core_window_t*	pressed{nullptr};				//The handle to a window which has been pressed by pressing left button of mouse.
				core_window_t*	pressed_by_space{ nullptr };	//The handle to a window which has been pressed by pressing spacebar.
				core_window_t*	hovered{nullptr};				//the latest window that mouse moved
			}condition;

			root_misc(core_window_t * wd, unsigned width, unsigned height)
				:	window(wd),
				root_graph({ width, height })
			{}
		};//end struct root_misc

		class root_register
		{
		public:
			root_misc* insert(native_window_type wd, const root_misc& misc)
			{
				recent_ = wd;
				auto ret = table_.insert(std::make_pair(wd, misc));
				misc_ptr_ = &(ret.first->second);
				return misc_ptr_;
			}

			root_misc * find(native_window_type wd)
			{
				if (wd == recent_)
					return misc_ptr_;

				recent_ = wd;
				
				auto i = table_.find(wd);
				if (i != table_.end())
					misc_ptr_ = &(i->second);
				else
					misc_ptr_ = nullptr;

				return misc_ptr_;
			}

			void erase(native_window_type wd)
			{
				table_.erase(wd);
				recent_ = wd;
				misc_ptr_ = nullptr;
			}
		private:
			//Cached
			mutable native_window_type	recent_{nullptr};
			mutable root_misc *			misc_ptr_{nullptr};

			std::map<native_window_type, root_misc> table_;
		};
	}
}//end namespace nana
#endif	//NANA_GUI_INNER_FWD_IMPLEMENT_HPP
