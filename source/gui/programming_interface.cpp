/*
 *	Nana GUI Programming Interface Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/programming_interface.cpp
 */

#include <nana/gui/programming_interface.hpp>
#include <nana/gui/detail/basic_window.hpp>
#include <nana/system/platform.hpp>
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/widgets/widget.hpp>
#include <algorithm>

namespace nana
{
	//restrict
	//		this name is only visible for this compiling-unit
	namespace restrict
	{
		namespace
		{
			typedef detail::bedrock::core_window_t core_window_t;
			typedef detail::bedrock::interface_type	interface_type;

			auto& bedrock = detail::bedrock::instance();
			detail::bedrock::window_manager_t& window_manager = bedrock.wd_manager;
		}
	}

	namespace effects
	{
		class effects_accessor
		{
		public:
			static bground_interface * create(const bground_factory_interface& factory)
			{
				return factory.create();
			}
		};
	}
namespace API
{
	namespace detail
	{
		general_events* get_general_events(window wd)
		{
			if (!restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)))
				return nullptr;

			return reinterpret_cast<restrict::core_window_t*>(wd)->together.attached_events;
		}
	}//end namespace detail

	void effects_edge_nimbus(window wd, effects::edge_nimbus en)
	{
		auto const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard isg;
		if(restrict::window_manager.available(iwd))
		{
			auto & cont = iwd->root_widget->other.attribute.root->effects_edge_nimbus;
			if(effects::edge_nimbus::none != en)
			{
				if (iwd->effect.edge_nimbus == effects::edge_nimbus::none)
				{
					restrict::core_window_t::edge_nimbus_action ena = { iwd };
					cont.push_back(ena);
				}
				iwd->effect.edge_nimbus = static_cast<effects::edge_nimbus>(static_cast<unsigned>(en) | static_cast<unsigned>(iwd->effect.edge_nimbus));
			}
			else
			{
				if(effects::edge_nimbus::none != iwd->effect.edge_nimbus)
				{
					for(auto i = cont.begin(); i != cont.end(); ++i)
						if(i->window == iwd)
						{
							cont.erase(i);
							break;
						}
				}
				iwd->effect.edge_nimbus = effects::edge_nimbus::none;
			}
		}
	}

	effects::edge_nimbus effects_edge_nimbus(window wd)
	{
		auto const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard isg;
		return (restrict::window_manager.available(iwd) ? iwd->effect.edge_nimbus : effects::edge_nimbus::none);
	}

	void effects_bground(window wd, const effects::bground_factory_interface& factory, double fade_rate)
	{	
		auto const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard isg;
		if(restrict::window_manager.available(iwd))
		{
			auto new_effect_ptr = effects::effects_accessor::create(factory);
			if(nullptr == new_effect_ptr)
				return;

			delete iwd->effect.bground;
			iwd->effect.bground = new_effect_ptr;
			iwd->effect.bground_fade_rate = fade_rate;
			restrict::window_manager.enable_effects_bground(iwd, true);
			API::refresh_window(wd);
		}
	}

	bground_mode effects_bground_mode(window wd)
	{
		auto const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard isg;
		if(restrict::window_manager.available(iwd) && iwd->effect.bground)
			return (iwd->effect.bground_fade_rate <= 0.009 ? bground_mode::basic : bground_mode::blend);

		return bground_mode::none;
	}

	void effects_bground_remove(window wd)
	{
		const auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard isg;
		if(restrict::window_manager.available(iwd))
		{
			if(restrict::window_manager.enable_effects_bground(iwd, false))
				API::refresh_window(wd);
		}
	}

	namespace dev
	{

		bool set_events(window wd, const std::shared_ptr<general_events>& gep)
		{
			auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard lock;
			if (restrict::window_manager.available(iwd) && iwd->set_events(gep))
			{
				restrict::bedrock.evt_operation.make(wd, gep);
				return true;
			}
			return false;
		}

		void attach_drawer(widget& wd, drawer_trigger& dr)
		{
			const auto iwd = reinterpret_cast<restrict::core_window_t*>(wd.handle());
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				iwd->drawer.graphics.make(iwd->dimension.width, iwd->dimension.height);
				iwd->drawer.graphics.rectangle(iwd->color.background, true);
				iwd->drawer.attached(wd, dr);
				iwd->drawer.refresh();	//Always redrawe no matter it is visible or invisible. This can make the graphics data correctly.
			}
		}

		nana::string window_caption(window wd)
		{
			auto const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;

			if(restrict::window_manager.available(iwd))
			{
				if(iwd->other.category == category::flags::root)
					return restrict::interface_type::window_caption(iwd->root);
				return iwd->title;
			}
			return {};
		}

		void window_caption(window wd, nana::string title)
		{
			auto const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard lock;
			if (restrict::window_manager.available(iwd))
			{
				iwd->title.swap(title);
				if (iwd->other.category == category::flags::root)
					restrict::interface_type::window_caption(iwd->root, iwd->title);

				restrict::window_manager.update(iwd, true, false);
			}
		}

		window create_window(window owner, bool nested, const rectangle& r, const appearance& ap, widget* wdg)
		{
			return reinterpret_cast<window>(restrict::window_manager.create_root(reinterpret_cast<restrict::core_window_t*>(owner), nested, r, ap, wdg));
		}

		window create_widget(window parent, const rectangle& r, widget* wdg)
		{
			return reinterpret_cast<window>(restrict::window_manager.create_widget(reinterpret_cast<restrict::core_window_t*>(parent), r, false, wdg));
		}

		window create_lite_widget(window parent, const rectangle& r, widget* wdg)
		{
			return reinterpret_cast<window>(restrict::window_manager.create_widget(reinterpret_cast<restrict::core_window_t*>(parent), r, true, wdg));
		}

		window create_frame(window parent, const rectangle& r, widget* wdg)
		{
			return reinterpret_cast<window>(restrict::window_manager.create_frame(reinterpret_cast<restrict::core_window_t*>(parent), r, wdg));
		}

		paint::graphics* window_graphics(window wd)
		{
			internal_scope_guard isg;
			if(restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)))
				return &reinterpret_cast<restrict::core_window_t*>(wd)->drawer.graphics;
			return nullptr;
		}
	}//end namespace dev

	//exit
	//close all windows in current thread
	void exit()
	{
		std::vector<restrict::core_window_t*> v;

		internal_scope_guard lock;
		restrict::window_manager.all_handles(v);
		if(v.size())
		{
			std::vector<native_window_type> roots;
			native_window_type root = nullptr;
			unsigned tid = nana::system::this_thread_id();
			for(auto wd : v)
			{
				if((wd->thread_id == tid) && (wd->root != root))
				{
					root = wd->root;
					if(roots.cend() == std::find(roots.cbegin(), roots.cend(), root))
						roots.push_back(root);
				}
			}

			for(auto i : roots)
				restrict::interface_type::close_window(i);
		}
	}

	//transform_shortkey_text
	//@brief:	This function searchs whether the text contains a '&' and removes the character for transforming.
	//			If the text contains more than one '&' charachers, the others are ignored. e.g
	//			text = "&&a&bcd&ef", the result should be "&abcdef", shortkey = 'b', and pos = 2.
	//@param, text: the text is transformed.
	//@param, shortkey: the character which indicates a short key.
	//@param, skpos: retrives the shortkey position if it is not a null_ptr;
	nana::string transform_shortkey_text(nana::string text, nana::string::value_type &shortkey, nana::string::size_type *skpos)
	{
		shortkey = 0;
		nana::string::size_type off = 0;
		while(true)
		{
			nana::string::size_type pos = text.find_first_of('&', off);
			if(pos != nana::string::npos)
			{
				text.erase(pos, 1);
				if(shortkey == 0 && pos < text.length())
				{
					shortkey = text.at(pos);
					if(shortkey == '&')	//This indicates the text contains "&&", it means the symbol have to be ignored.
						shortkey = 0;
					else if(skpos)
						*skpos = pos;
				}
				off = pos + 1;
			}
			else
				break;
		}
		return text;
	}

	bool register_shortkey(window wd, unsigned long key)
	{
		return restrict::window_manager.register_shortkey(reinterpret_cast<restrict::core_window_t*>(wd), key);
	}

	void unregister_shortkey(window wd)
	{
		restrict::window_manager.unregister_shortkey(reinterpret_cast<restrict::core_window_t*>(wd), false);
	}

	nana::size screen_size()
	{
		return restrict::interface_type::screen_size();
	}

	rectangle screen_area_from_point(const point& pos)
	{
		return restrict::interface_type::screen_area_from_point(pos);
	}

	nana::point	cursor_position()
	{
		return restrict::interface_type::cursor_position();
	}

	nana::rectangle make_center(unsigned width, unsigned height)
	{
		nana::size screen = restrict::interface_type::screen_size();
		nana::rectangle result(
			width > screen.width? 0: (screen.width - width)>>1,
			height > screen.height? 0: (screen.height - height)>> 1,
			width, height
		);

		return result;
	}

	nana::rectangle make_center(window wd, unsigned width, unsigned height)
	{
		nana::rectangle r = make_center(width, height);

		nana::point pos{ r.x, r.y };
		calc_window_point(wd, pos);
		r = pos;
		return r;
	}

	void window_icon_default(const paint::image& img)
	{
		restrict::window_manager.default_icon(img);
	}

	void window_icon(window wd, const paint::image& img)
	{
		restrict::window_manager.icon(reinterpret_cast<restrict::core_window_t*>(wd), img);
	}

	bool empty_window(window wd)
	{
		return (restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)) == false);
	}

	void enable_dropfiles(window wd, bool enb)
	{
		internal_scope_guard lock;
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		auto native_handle = API::root(wd);
		if (native_handle)
		{
			iwd->flags.dropable = enb;
			restrict::interface_type::enable_dropfiles(native_handle, enb);
		}
	}

	native_window_type root(window wd)
	{
		return restrict::bedrock.root(reinterpret_cast<restrict::core_window_t*>(wd));
	}

	window root(native_window_type wd)
	{
		return reinterpret_cast<window>(restrict::window_manager.root(wd));
	}

	void enable_double_click(window wd, bool dbl)
	{
		auto const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
			iwd->flags.dbl_click = dbl;
	}

	void fullscreen(window wd, bool v)
	{
		internal_scope_guard lock;
		if(restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)))
			reinterpret_cast<restrict::core_window_t*>(wd)->flags.fullscreen = v;
	}

	bool insert_frame(window frame, native_window_type native_window)
	{
		return restrict::window_manager.insert_frame(reinterpret_cast<restrict::core_window_t*>(frame), native_window);
	}

	native_window_type frame_container(window frame)
	{
		auto frm = reinterpret_cast<restrict::core_window_t*>(frame);
		internal_scope_guard lock;
		if (restrict::window_manager.available(frm) && (frm->other.category == category::flags::frame))
			return frm->other.attribute.frame->container;
		return nullptr;
	}

	native_window_type frame_element(window frame, unsigned index)
	{
		auto frm = reinterpret_cast<restrict::core_window_t*>(frame);
		internal_scope_guard lock;
		if (restrict::window_manager.available(frm) && (frm->other.category == category::flags::frame))
		{
			if (index < frm->other.attribute.frame->attach.size())
				return frm->other.attribute.frame->attach.at(index);
		}
		return nullptr;
	}

	void close_window(window wd)
	{
		restrict::window_manager.close(reinterpret_cast<restrict::core_window_t*>(wd));
	}

	void show_window(window wd, bool show)
	{
		restrict::window_manager.show(reinterpret_cast<restrict::core_window_t*>(wd), show);
	}

	bool visible(window wd)
	{
		auto const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
		{
			if(iwd->other.category == category::flags::root)
				return restrict::interface_type::is_window_visible(iwd->root);
			return iwd->visible;
		}
		return false;
	}

	void restore_window(window wd)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
		{
			if(iwd->other.category == category::flags::root)
				restrict::interface_type::restore_window(iwd->root);
		}
	}

	void zoom_window(window wd, bool ask_for_max)
	{
		auto core_wd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(core_wd))
		{
			if(category::flags::root == core_wd->other.category)
				restrict::interface_type::zoom_window(core_wd->root, ask_for_max);
		}
	}

	window get_parent_window(window wd)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
			return reinterpret_cast<window>(iwd->other.category == category::flags::root ? iwd->owner : iwd->parent);

		return nullptr;
	}

	window get_owner_window(window wd)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd) && (iwd->other.category == category::flags::root))
		{
			native_window_type owner = restrict::interface_type::get_owner_window(iwd->root);
			if(owner)
				return reinterpret_cast<window>(restrict::window_manager.root(owner));
		}

		return nullptr;
	}

	bool set_parent_window(window wd, window new_parent)
	{
		return restrict::bedrock.wd_manager.set_parent(reinterpret_cast<restrict::core_window_t*>(wd), reinterpret_cast<restrict::core_window_t*>(new_parent));
	}

	void umake_event(event_handle eh)
	{
		restrict::bedrock.evt_operation.erase(eh);
	}

	nana::point window_position(window wd)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
		{
			return ( (iwd->other.category == category::flags::root) ?
				restrict::interface_type::window_position(iwd->root) : iwd->pos_owner);
		}
		return nana::point{};
	}

	void move_window(window wd, int x, int y)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.move(iwd, x, y, false))
		{
			if(category::flags::root != iwd->other.category)
			{
				do{
					iwd = iwd->parent;
				} while (category::flags::lite_widget == iwd->other.category);
			}
			restrict::window_manager.update(iwd, false, false);
		}
	}

	void move_window(window wd, const rectangle& r)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.move(iwd, r))
		{
			if (category::flags::root != iwd->other.category)
			{
				do{
					iwd = iwd->parent;
				} while (category::flags::lite_widget == iwd->other.category);
			}
			restrict::window_manager.update(iwd, false, false);
		}
	}

	void bring_to_top(window wd)
	{
		restrict::interface_type::bring_to_top(root(wd));
	}

	bool set_window_z_order(window wd, window wd_after, z_order_action action_if_no_wd_after)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		native_window_type native_after = nullptr;
		internal_scope_guard lock;
		if (restrict::window_manager.available(iwd) && (category::flags::root == iwd->other.category))
		{
			if(wd_after)
			{
				auto iwd_after = reinterpret_cast<restrict::core_window_t*>(wd_after);
				if (restrict::window_manager.available(iwd_after) && (iwd_after->other.category == category::flags::root))
				{
					native_after = iwd_after->root;
					action_if_no_wd_after = z_order_action::none;
				}
				else
					return false;
			}
			restrict::interface_type::set_window_z_order(iwd->root, native_after, action_if_no_wd_after);
			return true;
		}
		return false;
	}

	nana::size window_size(window wd)
	{
		nana::rectangle r;
		API::window_rectangle(wd, r);
		return{ r.width, r.height };
	}

	void window_size(window wd, const size& sz)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.size(iwd, sz, false, false))
		{
			if (category::flags::root != iwd->other.category)
			{
				do{
					iwd = iwd->parent;
				} while (category::flags::lite_widget == iwd->other.category);
			}
			restrict::window_manager.update(iwd, false, false);
		}
	}

	bool window_rectangle(window wd, rectangle& r)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
		{
			r = rectangle(iwd->pos_owner, iwd->dimension);
			return true;
		}
		return false;
	}

	bool track_window_size(window wd, const nana::size& sz, bool true_for_max)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd) == false)
			return false;

		nana::size & ts = (true_for_max ? iwd->max_track_size : iwd->min_track_size);
		if(!sz.empty())
		{
			if(true_for_max)
			{
				//Make sure the new size is larger than min size
				if (iwd->min_track_size.width > sz.width || iwd->min_track_size.height > sz.height)
					return false;
			}
			else
			{
				//Make sure that the new size is less than max size
				if ((iwd->max_track_size.width || iwd->max_track_size.height) && (iwd->max_track_size.width < sz.width || iwd->max_track_size.height < sz.height))
					return false;
			}

			ts = restrict::interface_type::check_track_size(sz, iwd->extra_width, iwd->extra_height, true_for_max);
		}
		else
			ts.width = ts.height = 0;
		return true;
	}

	void window_enabled(window wd, bool enabled)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd) && (iwd->flags.enabled != enabled))
		{
			iwd->flags.enabled = enabled;
			restrict::window_manager.update(iwd, true, false);
			if(category::flags::root == iwd->other.category)
				restrict::interface_type::enable_window(iwd->root, enabled);
		}
	}

	bool window_enabled(window wd)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		return (restrict::window_manager.available(iwd) ? iwd->flags.enabled : false);
	}

	//lazy_refresh:
	//@brief: A widget drawer draws the widget surface in answering an event. This function will tell the drawer to copy the graphics into window after event answering.
	void lazy_refresh()
	{
		restrict::bedrock.thread_context_lazy_refresh();
	}

	//refresh_window
	//@brief: Refresh the window and display it immediately.
	void refresh_window(window wd)
	{
		restrict::window_manager.update(reinterpret_cast<restrict::core_window_t*>(wd), true, false);
	}

	void refresh_window_tree(window wd)
	{
		restrict::window_manager.refresh_tree(reinterpret_cast<restrict::core_window_t*>(wd));
	}

	//update_window
	//@brief: it displays a window immediately without refreshing.
	void update_window(window wd)
	{
		restrict::window_manager.update(reinterpret_cast<restrict::core_window_t*>(wd), false, true);
	}

	void window_caption(window wd, const nana::string& title)
	{
		auto const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
			restrict::window_manager.signal_fire_caption(iwd, title.c_str());
	}
	
	nana::string window_caption(window wd)
	{
		auto const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
			return restrict::window_manager.signal_fire_caption(iwd);

		return{};
	}

	void window_cursor(window wd, cursor cur)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
		{
			iwd->predef_cursor = cur;
			restrict::bedrock.update_cursor(iwd);
		}
	}

	cursor window_cursor(window wd)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
			return iwd->predef_cursor;

		return cursor::arrow;
	}

	bool is_focus_window(window wd)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
			return (iwd->root_widget->other.attribute.root->focus == iwd);

		return false;
	}

	void activate_window(window wd)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if (restrict::window_manager.available(iwd))
		{
			if(iwd->flags.take_active)
				restrict::interface_type::activate_window(iwd->root);
		}
	}

	window focus_window()
	{
		internal_scope_guard lock;
		return reinterpret_cast<window>(restrict::bedrock.focus());
	}

	void focus_window(window wd)
	{
		restrict::window_manager.set_focus(reinterpret_cast<restrict::core_window_t*>(wd), false);
		restrict::window_manager.update(reinterpret_cast<restrict::core_window_t*>(wd), false, false);
	}

	window capture_window()
	{
		return reinterpret_cast<window>(restrict::window_manager.capture_window());
	}

	window capture_window(window wd, bool value)
	{
		return reinterpret_cast<window>(
					restrict::window_manager.capture_window(reinterpret_cast<restrict::core_window_t*>(wd), value)
		);
	}

	void capture_ignore_children(bool ignore)
	{
		restrict::window_manager.capture_ignore_children(ignore);
	}

	void modal_window(window wd)
	{
		{
			auto const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;

			if (!restrict::window_manager.available(iwd))
				return;

			if ((iwd->other.category == category::flags::root) && (iwd->flags.modal == false))
			{
				iwd->flags.modal = true;
#if defined(NANA_X11)
				restrict::interface_type::set_modal(iwd->root);
#endif
				restrict::window_manager.show(iwd, true);
			}
			else
				return;
		}

		//modal has to guarantee that does not lock the mutex of window_manager before invokeing the pump_event,
		//otherwise, the modal will prevent the other thread access the window.
		restrict::bedrock.pump_event(wd, true);
	}

	void wait_for(window wd)
	{
		if (wd)
			restrict::bedrock.pump_event(wd, false);
	}

	nana::color_t foreground(window wd)
	{
		internal_scope_guard lock;
		if(restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)))
			return reinterpret_cast<restrict::core_window_t*>(wd)->color.foreground;
		return 0;
	}

	color_t foreground(window wd, color_t col)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
		{
			color_t prev = iwd->color.foreground;
			if(prev != col)
			{
				iwd->color.foreground = col;
				restrict::window_manager.update(iwd, true, false);
			}
			return prev;
		}
		return 0;
	}

	color_t background(window wd)
	{
		internal_scope_guard lock;
		if(restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)))
			return reinterpret_cast<restrict::core_window_t*>(wd)->color.background;
		return 0;
	}

	color_t background(window wd, color_t col)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
		{
			color_t prev = iwd->color.background;
			if(prev != col)
			{
				iwd->color.background = col;
				restrict::window_manager.update(iwd, true, false);
			}
			return prev;
		}
		return 0;
	}

	color_t active(window wd)
	{
		internal_scope_guard lock;
		if(restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)))
			return reinterpret_cast<restrict::core_window_t*>(wd)->color.active;
		return 0;
	}

	color_t active(window wd, color_t col)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
		{
			color_t prev = iwd->color.active;
			if(prev != col)
			{
				iwd->color.active = col;
				restrict::window_manager.update(iwd, true, false);
			}
			return prev;
		}
		
		return 0;
	}

	void create_caret(window wd, unsigned width, unsigned height)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd) && (nullptr == iwd->together.caret))
			iwd->together.caret = new ::nana::detail::caret_descriptor(iwd, width, height);
	}

	void destroy_caret(window wd)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
		{
			auto p = iwd->together.caret;
			iwd->together.caret = nullptr;
			delete p;
		}
	}

	void caret_pos(window wd, int x, int y)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd) && iwd->together.caret)
			iwd->together.caret->position(x, y);
	}

	nana::point caret_pos(window wd)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd) && iwd->together.caret)
			return iwd->together.caret->position();

		return{};
	}

	void caret_effective_range(window wd, const nana::rectangle& rect)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd) && iwd->together.caret)
			iwd->together.caret->effective_range(rect);
	}

	void caret_size(window wd, const nana::size& sz)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard isg;
		if(restrict::window_manager.available(iwd) && iwd->together.caret)
			iwd->together.caret->size(sz);
	}

	nana::size caret_size(window wd)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd) && iwd->together.caret)
			return iwd->together.caret->size();

		return{};
	}

	void caret_visible(window wd, bool is_show)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd) && iwd->together.caret)
			iwd->together.caret->visible(is_show);
	}

	bool caret_visible(window wd)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd) && iwd->together.caret)
			return iwd->together.caret->visible();

		return false;
	}

	void tabstop(window wd)
	{
		restrict::window_manager.enable_tabstop(reinterpret_cast<restrict::core_window_t*>(wd));
	}

	//eat_tabstop
	//@brief: set a eating tab window that it processes a pressing of tab itself
	void eat_tabstop(window wd, bool eat)
	{
		if(wd)
		{
			auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
			internal_scope_guard isg;
			if(restrict::window_manager.available(iwd))
			{
				if(eat)
					iwd->flags.tab |= ::nana::detail::tab_type::eating;
				else
					iwd->flags.tab &= ~::nana::detail::tab_type::eating;
			}
		}
	}

	window move_tabstop(window wd, bool next)
	{
		restrict::core_window_t* ts_wd = restrict::window_manager.tabstop(reinterpret_cast<restrict::core_window_t*>(wd), next);
		restrict::window_manager.set_focus(ts_wd, false);
		restrict::window_manager.update(ts_wd, false, false);
		return reinterpret_cast<window>(ts_wd);
	}

	//glass_window deprecated
	//@brief: Test a window whether it is a glass attribute.
	bool glass_window(window wd)
	{
		return (bground_mode::basic == effects_bground_mode(wd));
	}

	bool glass_window(window wd, bool isglass)	//deprecated
	{
		if(isglass)
			effects_bground(wd, effects::bground_transparent(0), 0);
		else
			effects_bground_remove(wd);
		return true;
	}

	void take_active(window wd, bool active, window take_if_active_false)
	{
		auto const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		auto take_if_false = reinterpret_cast<restrict::core_window_t*>(take_if_active_false);

		internal_scope_guard lock;
		if (restrict::window_manager.available(iwd))
		{
			if (active || (take_if_false && (restrict::window_manager.available(take_if_false) == false)))
				take_if_false = 0;

			iwd->flags.take_active = active;
			iwd->other.active_window = take_if_false;
		}
	}

	bool window_graphics(window wd, nana::paint::graphics& graph)
	{
		return restrict::window_manager.get_graphics(reinterpret_cast<restrict::core_window_t*>(wd), graph);
	}

	bool root_graphics(window wd, nana::paint::graphics& graph)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
		{
			graph = *(iwd->root_graph);
			return true;
		}
		return false;
	}

	bool get_visual_rectangle(window wd, nana::rectangle& r)
	{
		return restrict::window_manager.get_visual_rectangle(reinterpret_cast<restrict::core_window_t*>(wd), r);
	}

	void typeface(window wd, const nana::paint::font& font)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
		{
			iwd->drawer.graphics.typeface(font);
			iwd->drawer.typeface_changed();
			restrict::window_manager.update(iwd, true, false);
		}
	}

	nana::paint::font typeface(window wd)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
			return iwd->drawer.graphics.typeface();

		return{};
	}

	bool calc_screen_point(window wd, nana::point& pos)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
		{
			pos += iwd->pos_root;
			return restrict::interface_type::calc_screen_point(iwd->root, pos);
		}
		return false;
	}

	bool calc_window_point(window wd, nana::point& pos)
	{
		return restrict::window_manager.calc_window_point(reinterpret_cast<restrict::core_window_t*>(wd), pos);
	}

	window find_window(const nana::point& pos)
	{
		auto wd = restrict::interface_type::find_window(pos.x, pos.y);
		if(wd)
		{
			nana::point clipos(pos.x, pos.y);
			restrict::interface_type::calc_window_point(wd, clipos);
			return reinterpret_cast<window>(
						restrict::window_manager.find_window(wd, clipos.x, clipos.y));
		}
		return nullptr;
	}

	void register_menu_window(window wd, bool has_keyboard)
	{
		internal_scope_guard lock;
		if(restrict::window_manager.available(reinterpret_cast<restrict::core_window_t*>(wd)))
			restrict::bedrock.set_menu(reinterpret_cast<restrict::core_window_t*>(wd)->root, has_keyboard);
	}

	bool attach_menubar(window menubar)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(menubar);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd) && (nullptr == iwd->root_widget->other.attribute.root->menubar))
		{
			iwd->root_widget->other.attribute.root->menubar = iwd;
			return true;
		}
		return false;
	}

	void detach_menubar(window menubar)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(menubar);
		internal_scope_guard lock;
		if (restrict::window_manager.available(iwd))
		{
			if (iwd->root_widget->other.attribute.root->menubar == iwd)
				iwd->root_widget->other.attribute.root->menubar = nullptr;
		}
	}

	void restore_menubar_taken_window()
	{
		auto wd = restrict::bedrock.get_menubar_taken();
		if(wd)
		{
			internal_scope_guard lock;
			restrict::window_manager.set_focus(wd, false);
			restrict::window_manager.update(wd, true, false);
		}
	}

	bool is_window_zoomed(window wd, bool ask_for_max)
	{
		auto const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if (restrict::window_manager.available(iwd))
		{
			if (iwd->other.category == nana::category::flags::root)
				return ::nana::detail::bedrock::interface_type::is_window_zoomed(iwd->root, ask_for_max);
		}
		return false;
	}

	void widget_borderless(window wd, bool enabled)
	{
		auto const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if (restrict::window_manager.available(iwd))
		{
			if ((category::flags::widget == iwd->other.category) && (iwd->flags.borderless != enabled))
			{
				iwd->flags.borderless = enabled;
				restrict::window_manager.update(iwd, true, false);
			}
		}
	}

	bool widget_borderless(window wd)
	{
		auto const iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if (restrict::window_manager.available(iwd))
			return iwd->flags.borderless;

		return false;
	}

	nana::mouse_action mouse_action(window wd)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
			return iwd->flags.action;
		return nana::mouse_action::normal;
	}

	nana::element_state element_state(window wd)
	{
		auto iwd = reinterpret_cast<restrict::core_window_t*>(wd);
		internal_scope_guard lock;
		if(restrict::window_manager.available(iwd))
		{
			const bool is_focused = (iwd->root_widget->other.attribute.root->focus == iwd);
			switch(iwd->flags.action)
			{
			case nana::mouse_action::normal:
				return (is_focused ? nana::element_state::focus_normal : nana::element_state::normal);
			case nana::mouse_action::over:
				return (is_focused ? nana::element_state::focus_hovered : nana::element_state::hovered);
			case nana::mouse_action::pressed:
				return nana::element_state::pressed;
			default:
				if(false == iwd->flags.enabled)
					return nana::element_state::disabled;
			}
		}
		return nana::element_state::normal;
	}
}//end namespace API
}//end namespace nana
