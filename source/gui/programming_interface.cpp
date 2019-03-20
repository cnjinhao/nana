/*
 *	Nana GUI Programming Interface Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/programming_interface.cpp
 *	@author: Jinhao
 */

#include <nana/gui/programming_interface.hpp>
#include <nana/gui/detail/bedrock.hpp>
#include <nana/gui/detail/basic_window.hpp>
#include <nana/gui/detail/window_manager.hpp>
#include <nana/gui/detail/window_layout.hpp>
#include <nana/system/platform.hpp>
#include <nana/gui/detail/native_window_interface.hpp>
#include <nana/gui/widgets/widget.hpp>
#include <nana/gui/detail/events_operation.hpp>

#include "../../source/detail/platform_abstraction.hpp"
#ifdef NANA_X11
#	include "../../source/detail/posix/platform_spec.hpp"
#endif

namespace nana
{
	//restrict
	//		this name is only visible for this compiling-unit
	namespace restrict
	{
		namespace
		{
			auto& bedrock = detail::bedrock::instance();

			inline detail::window_manager& wd_manager()
			{
				return bedrock.wd_manager();
			}
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
#ifdef NANA_X11
	//Some platform specific functions for X11
	namespace x11
	{
		/// Returns the connection to the X server
		const void* get_display()
		{
			auto & spec = nana::detail::platform_spec::instance();
			return spec.open_display();			
		}
	}
#endif

	using basic_window = ::nana::detail::basic_window;
	using interface_type = ::nana::detail::native_interface;

	namespace detail
	{
		::nana::widget_geometrics* make_scheme(::nana::detail::scheme_factory_interface&& factory)
		{
			return restrict::bedrock.scheme().create(static_cast<::nana::detail::scheme_factory_interface&&>(factory));
		}

		bool emit_event(event_code evt_code, window wd, const ::nana::event_arg& arg)
		{
			return restrict::bedrock.emit(evt_code, reinterpret_cast<::nana::detail::basic_window*>(wd), arg, true, restrict::bedrock.get_thread_context(), false);
		}

		bool emit_internal_event(event_code evt_code, window wd, const ::nana::event_arg& arg)
		{
			return restrict::bedrock.emit(evt_code, reinterpret_cast<::nana::detail::basic_window*>(wd), arg, true, restrict::bedrock.get_thread_context(), true);
		}

		void enum_widgets_function_base::enum_widgets(window wd, bool recursive)
		{
			auto iwd = reinterpret_cast<nana::detail::basic_window*>(wd);

			internal_scope_guard lock;
			if (restrict::wd_manager().available(iwd))
			{
				//Use a copy, because enum function may close a child window and the original children container would be changed,
				//in the situation, the walking thorugh directly to the iwd->children would cause error. 
				auto children = iwd->children;

				for (auto child : children)
				{
					auto widget_ptr = API::get_widget(reinterpret_cast<window>(child));
					if (!widget_ptr)
						continue;

					_m_enum_fn(widget_ptr);
					if (recursive)
						enum_widgets(reinterpret_cast<window>(child), recursive);
				}
			}
		}

		general_events* get_general_events(window wd)
		{
			if (!restrict::wd_manager().available(reinterpret_cast<basic_window*>(wd)))
				return nullptr;

			return reinterpret_cast<basic_window*>(wd)->annex.events_ptr.get();
		}
	}//end namespace detail

	void effects_edge_nimbus(window wd, effects::edge_nimbus en)
	{
		auto const iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard isg;
		if(restrict::wd_manager().available(iwd))
		{
			auto & cont = iwd->root_widget->other.attribute.root->effects_edge_nimbus;
			if(effects::edge_nimbus::none != en)
			{
				if (iwd->effect.edge_nimbus == effects::edge_nimbus::none)
				{
					cont.emplace_back(basic_window::edge_nimbus_action{ iwd, false});
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
		auto const iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard isg;
		return (restrict::wd_manager().available(iwd) ? iwd->effect.edge_nimbus : effects::edge_nimbus::none);
	}

	void effects_bground(window wd, const effects::bground_factory_interface& factory, double fade_rate)
	{
		if (fade_rate < 0.0 || fade_rate > 1.0)
			throw std::invalid_argument("effects_bground: value range of fade_rate must be [0, 1].");
		auto const iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard isg;
		if(restrict::wd_manager().available(iwd))
		{
			auto new_effect_ptr = effects::effects_accessor::create(factory);
			if(nullptr == new_effect_ptr)
				return;

			delete iwd->effect.bground;
			iwd->effect.bground = new_effect_ptr;
			iwd->effect.bground_fade_rate = fade_rate;
			restrict::wd_manager().enable_effects_bground(iwd, true);

			if (fade_rate < 0.01)
				iwd->flags.make_bground_declared = true;

			API::refresh_window(wd);
		}
	}

	void effects_bground(std::initializer_list<window> wdgs, const effects::bground_factory_interface& factory, double fade_rate)
	{
		for (auto wd : wdgs)
			effects_bground(wd, factory, fade_rate);
	}

	bground_mode effects_bground_mode(window wd)
	{
		auto const iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard isg;
		if(restrict::wd_manager().available(iwd) && iwd->effect.bground)
			return (iwd->effect.bground_fade_rate <= 0.009 ? bground_mode::basic : bground_mode::blend);

		return bground_mode::none;
	}

	void effects_bground_remove(window wd)
	{
		const auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard isg;
		if(restrict::wd_manager().available(iwd))
		{
			if(restrict::wd_manager().enable_effects_bground(iwd, false))
				API::refresh_window(wd);
		}
	}

	namespace dev
	{

		void affinity_execute(window window_handle, const std::function<void()>& fn)
		{
			interface_type::affinity_execute(root(window_handle), fn);
		}

		bool set_events(window wd, const std::shared_ptr<general_events>& gep)
		{
			auto iwd = reinterpret_cast<basic_window*>(wd);
			internal_scope_guard lock;

			if (restrict::wd_manager().available(iwd))
				iwd->set_events(gep);
			return false;
			
		}

		void set_scheme(window wd, widget_geometrics* wdg_geom)
		{
			auto iwd = reinterpret_cast<basic_window*>(wd);
			internal_scope_guard lock;
			if (restrict::wd_manager().available(iwd))
				iwd->annex.scheme = wdg_geom;
		}

		widget_geometrics* get_scheme(window wd)
		{
			auto iwd = reinterpret_cast<basic_window*>(wd);
			internal_scope_guard lock;
			return (restrict::wd_manager().available(iwd) ? iwd->annex.scheme : nullptr);
		}

		void set_measurer(window wd, ::nana::dev::widget_content_measurer_interface* measurer)
		{
			auto iwd = reinterpret_cast<basic_window*>(wd);
			internal_scope_guard lock;
			if (restrict::wd_manager().available(iwd))
				iwd->annex.content_measurer = measurer;
		}

		void attach_drawer(widget& wd, drawer_trigger& dr)
		{
			const auto iwd = reinterpret_cast<basic_window*>(wd.handle());
			internal_scope_guard isg;
			if(restrict::wd_manager().available(iwd))
			{
				iwd->drawer.graphics.make(iwd->dimension);
				iwd->drawer.graphics.rectangle(true, iwd->annex.scheme->background.get_color());
				iwd->drawer.attached(wd, dr);
				iwd->drawer.refresh();	//Always redrawe no matter it is visible or invisible. This can make the graphics data correctly.
			}
		}

		::nana::detail::native_string_type window_caption(window wd) noexcept
		{
			auto const iwd = reinterpret_cast<basic_window*>(wd);
			internal_scope_guard isg;

			if(restrict::wd_manager().available(iwd))
			{
				if (category::flags::root == iwd->other.category)
					return interface_type::window_caption(iwd->root);
				return iwd->title;
			}
			return {};
		}

		void window_caption(window wd, ::nana::detail::native_string_type title)
		{
			auto const iwd = reinterpret_cast<basic_window*>(wd);
			internal_scope_guard lock;
			if (restrict::wd_manager().available(iwd))
			{
				iwd->title.swap(title);
				if (iwd->other.category == category::flags::root)
					interface_type::window_caption(iwd->root, iwd->title);

				restrict::wd_manager().update(iwd, true, false);
			}
		}

		window create_window(window owner, bool nested, const rectangle& r, const appearance& ap, widget* wdg)
		{
			return reinterpret_cast<window>(restrict::wd_manager().create_root(reinterpret_cast<basic_window*>(owner), nested, r, ap, wdg));
		}

		window create_widget(window parent, const rectangle& r, widget* wdg)
		{
			return reinterpret_cast<window>(restrict::wd_manager().create_widget(reinterpret_cast<basic_window*>(parent), r, false, wdg));
		}

		window create_lite_widget(window parent, const rectangle& r, widget* wdg)
		{
			return reinterpret_cast<window>(restrict::wd_manager().create_widget(reinterpret_cast<basic_window*>(parent), r, true, wdg));
		}

		paint::graphics* window_graphics(window wd)
		{
			internal_scope_guard isg;
			if(restrict::wd_manager().available(reinterpret_cast<basic_window*>(wd)))
				return &reinterpret_cast<basic_window*>(wd)->drawer.graphics;
			return nullptr;
		}

		void delay_restore(bool enable)
		{
			restrict::bedrock.delay_restore(enable ? 0 : 1);
		}

		void register_menu_window(window wd, bool has_keyboard)
		{
			internal_scope_guard lock;
			if (restrict::wd_manager().available(reinterpret_cast<basic_window*>(wd)))
				restrict::bedrock.set_menu(reinterpret_cast<basic_window*>(wd)->root, has_keyboard);
		}

		void set_menubar(window wd, bool attach)
		{
			auto iwd = reinterpret_cast<basic_window*>(wd);
			internal_scope_guard lock;
			if (restrict::wd_manager().available(iwd))
			{
				auto root_attr = iwd->root_widget->other.attribute.root;
				if (attach)
				{
					if (!root_attr->menubar)
						root_attr->menubar = iwd;
				}
				else
				{
					if (iwd == root_attr->menubar)
						root_attr->menubar = nullptr;
				}
			}
		}

		void enable_space_click(window wd, bool enable)
		{
			auto iwd = reinterpret_cast<basic_window*>(wd);
			internal_scope_guard lock;
			if (restrict::wd_manager().available(iwd))
				iwd->flags.space_click_enabled = enable;
		}

		bool copy_transparent_background(window wd, paint::graphics& graph)
		{
			auto & buf = reinterpret_cast<basic_window*>(wd)->other.glass_buffer;
			internal_scope_guard lock;

			if (bground_mode::basic != API::effects_bground_mode(wd))
				return false;

			buf.paste(rectangle{ buf.size() }, graph, 0, 0);

			return true;
		}

		bool copy_transparent_background(window wd, const rectangle& src_r, paint::graphics& graph, const point& dst_pt)
		{
			auto iwd = reinterpret_cast<basic_window*>(wd);
			internal_scope_guard lock;

			if (bground_mode::basic != API::effects_bground_mode(wd))
				return false;
			
			iwd->other.glass_buffer.paste(src_r, graph, dst_pt.x, dst_pt.y);

			return true;
		}

		void lazy_refresh()
		{
			restrict::bedrock.thread_context_lazy_refresh();
		}

		void draw_shortkey_underline(paint::graphics& graph, const std::string& text, wchar_t shortkey, std::size_t shortkey_position, const point& text_pos, const color& line_color)
		{
			if (shortkey)
			{
#ifdef _nana_std_has_string_view
				auto off_x = (shortkey_position ? graph.text_extent_size(std::string_view{ text.c_str(), shortkey_position }).width : 0);
				auto key_px = static_cast<int>(graph.text_extent_size(std::wstring_view{ &shortkey, 1 }).width);
#else
				auto off_x = (shortkey_position ? graph.text_extent_size(text.c_str(), shortkey_position).width : 0);
				auto key_px = static_cast<int>(graph.text_extent_size(&shortkey, 1).width);
#endif

				unsigned ascent, descent, inleading;
				graph.text_metrics(ascent, descent, inleading);

				int x = text_pos.x + static_cast<int>(off_x);
				int y = text_pos.y + static_cast<int>(ascent + 2);

				graph.line({ x, y }, {x + key_px - 1, y}, line_color);

			}
		}

		void window_draggable(window wd, bool enabled)
		{
			auto real_wd = reinterpret_cast<basic_window*>(wd);
			internal_scope_guard lock;
			if (restrict::wd_manager().available(real_wd))
				real_wd->flags.draggable = enabled;
		}

		bool window_draggable(window wd)
		{
			auto real_wd = reinterpret_cast<basic_window*>(wd);
			internal_scope_guard lock;
			if (restrict::wd_manager().available(real_wd))
				return real_wd->flags.draggable;

			return false;
		}


	}//end namespace dev

	widget* get_widget(window wd)
	{
		internal_scope_guard lock;
		if (restrict::wd_manager().available(reinterpret_cast<basic_window*>(wd)))
			return reinterpret_cast<basic_window*>(wd)->widget_notifier->widget_ptr();

		return nullptr;
	}

	//close all windows in current thread
	void exit()
	{
		internal_scope_guard lock;
		restrict::bedrock.close_thread_window(nana::system::this_thread_id());
	}

	//close all windows  
	void exit_all()
	{
		internal_scope_guard lock;
		restrict::bedrock.close_thread_window(0);
	}

	//transform_shortkey_text
	//@brief:	This function searchs whether the text contains a '&' and removes the character for transforming.
	//			If the text contains more than one '&' charachers, the others are ignored. e.g
	//			text = "&&a&bcd&ef", the result should be "&abcdef", shortkey = 'b', and pos = 2.
	//@param, text: the text is transformed.
	//@param, shortkey: the character which indicates a short key.
	//@param, skpos: retrives the shortkey position if it is not a null_ptr;
	std::string transform_shortkey_text(std::string text, wchar_t &shortkey, std::string::size_type *skpos)
	{
		shortkey = 0;
		std::string::size_type off = 0;
		while(true)
		{
			auto pos = text.find_first_of('&', off);
			if(pos != std::wstring::npos)
			{
				text.erase(pos, 1);
				if(shortkey == 0 && pos < text.length())
				{
					shortkey = utf::char_at(text.c_str() + pos, 0, nullptr);
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
		return restrict::wd_manager().register_shortkey(reinterpret_cast<basic_window*>(wd), key);
	}

	void unregister_shortkey(window wd)
	{
		restrict::wd_manager().unregister_shortkey(reinterpret_cast<basic_window*>(wd), false);
	}

	::nana::point	cursor_position()
	{
		return interface_type::cursor_position();
	}

	::nana::rectangle make_center(unsigned width, unsigned height)
	{
		auto screen = interface_type::primary_monitor_size();
		return{
			static_cast<int>(width > screen.width ? 0 : (screen.width - width) >> 1),
			static_cast<int>(height > screen.height ? 0 : (screen.height - height) >> 1),
			width, height
		};
	}

	::nana::rectangle make_center(window wd, unsigned width, unsigned height)
	{
		nana::rectangle r = make_center(width, height);

		nana::point pos{ r.x, r.y };
		calc_window_point(wd, pos);
		r.position(pos);
		return r;
	}

	void window_icon_default(const paint::image& small_icon, const paint::image& big_icon)
	{
		restrict::wd_manager().icon(nullptr, small_icon, big_icon);
	}

	void window_icon(window wd, const paint::image& small_icon, const paint::image& big_icon)
	{
		if(nullptr != wd)
			restrict::wd_manager().icon(reinterpret_cast<basic_window*>(wd), small_icon, big_icon);
	}

	bool empty_window(window wd)
	{
		return (restrict::wd_manager().available(reinterpret_cast<basic_window*>(wd)) == false);
	}

	bool is_window(window wd)
	{
		return restrict::wd_manager().available(reinterpret_cast<basic_window*>(wd));
	}

	bool is_destroying(window wd)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (!restrict::wd_manager().available(iwd))
			return false;

		return iwd->flags.destroying;
	}

	void enable_dropfiles(window wd, bool enb)
	{
		internal_scope_guard lock;
		auto iwd = reinterpret_cast<basic_window*>(wd);
		auto native_handle = API::root(wd);
		if (native_handle)
		{
			iwd->flags.dropable = enb;
			interface_type::enable_dropfiles(native_handle, enb);
		}
	}

	bool is_transparent_background(window wd)
	{
		return (bground_mode::basic == effects_bground_mode(wd));
	}

	native_window_type root(window wd)
	{
		internal_scope_guard lock;
		if(is_window(wd))
			return reinterpret_cast<basic_window*>(wd)->root;
		return nullptr;
	}

	window root(native_window_type wd)
	{
		return reinterpret_cast<window>(restrict::wd_manager().root(wd));
	}

	void enable_double_click(window wd, bool dbl)
	{
		auto const iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd))
			iwd->flags.dbl_click = dbl;
	}

	void fullscreen(window wd, bool v)
	{
		internal_scope_guard lock;
		if(restrict::wd_manager().available(reinterpret_cast<basic_window*>(wd)))
			reinterpret_cast<basic_window*>(wd)->flags.fullscreen = v;
	}

	void close_window(window wd)
	{
		restrict::wd_manager().close(reinterpret_cast<basic_window*>(wd));
	}

	void show_window(window wd, bool show)
	{
		restrict::wd_manager().show(reinterpret_cast<basic_window*>(wd), show);
	}

	bool visible(window wd)
	{
		auto const iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd))
		{
			if(iwd->other.category == category::flags::root)
				return interface_type::is_window_visible(iwd->root);
			return iwd->visible;
		}
		return false;
	}

	void restore_window(window wd)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd))
		{
			if(iwd->other.category == category::flags::root)
				interface_type::restore_window(iwd->root);
		}
	}

	void zoom_window(window wd, bool ask_for_max)
	{
		auto core_wd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(core_wd))
		{
			if(category::flags::root == core_wd->other.category)
				interface_type::zoom_window(core_wd->root, ask_for_max);
		}
	}

	window get_parent_window(window wd)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd))
		{
			if (category::flags::root == iwd->other.category)
			{
				return reinterpret_cast<window>(restrict::wd_manager().root(
					interface_type::get_window(iwd->root, window_relationship::parent)
				));
			}
			return reinterpret_cast<window>(iwd->parent);
		}

		return nullptr;
	}

	window get_owner_window(window wd)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd) && (iwd->other.category == category::flags::root))
		{
			auto owner = interface_type::get_window(iwd->root, window_relationship::owner);
			if(owner)
				return reinterpret_cast<window>(restrict::wd_manager().root(owner));
		}

		return nullptr;
	}

	bool set_parent_window(window wd, window new_parent)
	{
		return restrict::wd_manager().set_parent(reinterpret_cast<basic_window*>(wd), reinterpret_cast<basic_window*>(new_parent));
	}

	void umake_event(event_handle eh)
	{
		restrict::bedrock.evt_operation().erase(eh);
	}

	nana::point window_position(window wd)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd))
		{
			return ( (iwd->other.category == category::flags::root) ?
				interface_type::window_position(iwd->root) : iwd->pos_owner);
		}
		return nana::point{};
	}

	void move_window(window wd, const point& pos)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().move(iwd, pos.x, pos.y, false))
		{
			basic_window* update_wd = nullptr;
			if (iwd->displayed() && iwd->effect.bground)
			{
				update_wd = iwd;
				restrict::wd_manager().update(iwd, true, false);
			}

			basic_window* anc = iwd;
			if (category::flags::root != iwd->other.category)
				anc = iwd->seek_non_lite_widget_ancestor();

			if (anc != update_wd)
				restrict::wd_manager().update(anc, false, false);
		}
	}

	void move_window(window wd, const rectangle& r)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().move(iwd, r))
		{
			if (category::flags::root != iwd->other.category)
				iwd = iwd->seek_non_lite_widget_ancestor();

			restrict::wd_manager().update(iwd, false, false);
		}
	}

	void bring_top(window wd, bool activated)
	{
		interface_type::bring_top(root(wd), activated);
	}

	bool set_window_z_order(window wd, window wd_after, z_order_action action_if_no_wd_after)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		native_window_type native_after = nullptr;
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd) && (category::flags::root == iwd->other.category))
		{
			if(wd_after)
			{
				auto iwd_after = reinterpret_cast<basic_window*>(wd_after);
				if (restrict::wd_manager().available(iwd_after) && (iwd_after->other.category == category::flags::root))
				{
					native_after = iwd_after->root;
					action_if_no_wd_after = z_order_action::none;
				}
				else
					return false;
			}
			interface_type::set_window_z_order(iwd->root, native_after, action_if_no_wd_after);
			return true;
		}
		return false;
	}

	void draw_through(window wd, std::function<void()> draw_fn)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (!restrict::wd_manager().available(iwd))
			throw std::invalid_argument("draw_through: invalid window parameter");

		if (::nana::category::flags::root != iwd->other.category)
			throw std::invalid_argument("draw_through: the window is not a root widget");

		iwd->other.attribute.root->draw_through.swap(draw_fn);
	}

	void map_through_widgets(window wd, native_drawable_type drawable)
	{
		auto iwd = reinterpret_cast<::nana::detail::basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd) && iwd->is_draw_through() )
			restrict::bedrock.map_through_widgets(iwd, drawable);
	}

	nana::size window_size(window wd)
	{
		nana::rectangle r;
		API::get_window_rectangle(wd, r);
		return{ r.width, r.height };
	}

	void window_size(window wd, const size& sz)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().size(iwd, sz, false, false))
		{
			if (category::flags::root != iwd->other.category)
				iwd = iwd->seek_non_lite_widget_ancestor();

			restrict::wd_manager().update(iwd, false, false);
		}
	}

	::nana::size window_outline_size(window wd)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (!restrict::wd_manager().available(iwd))
			return{};

		auto sz = window_size(wd);

		if(category::flags::root == iwd->other.category)
		{
			auto fm_extents = interface_type::window_frame_extents(iwd->root);
			sz.width += fm_extents.left + fm_extents.right;
			sz.height += fm_extents.top + fm_extents.bottom;
		}

		return sz;
	}

	void window_outline_size(window wd, const size& sz)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd))
		{
			if (category::flags::root == iwd->other.category)
			{
				auto fm_extents = interface_type::window_frame_extents(iwd->root);

				size inner_size = sz;

				if (inner_size.width < static_cast<unsigned>(fm_extents.left + fm_extents.right))
					inner_size.width = 0;
				else
					inner_size.width -= static_cast<unsigned>(fm_extents.left + fm_extents.right);

				if (inner_size.height < static_cast<unsigned>(fm_extents.top + fm_extents.bottom))
					inner_size.height = 0;
				else
					inner_size.height -= static_cast<unsigned>(fm_extents.top + fm_extents.bottom);			

				window_size(wd, inner_size);
			}
			else
				window_size(wd, sz);
		}
	}

	std::optional<rectangle> window_rectangle(window wd)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd))
			return rectangle(iwd->pos_owner, iwd->dimension);
		return{};
	}

	bool get_window_rectangle(window wd, rectangle& r)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd))
		{
			r = rectangle(iwd->pos_owner, iwd->dimension);
			return true;
		}
		return false;
	}

	bool track_window_size(window wd, const nana::size& sz, bool true_for_max)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd) == false)
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

			ts = interface_type::check_track_size(sz, iwd->extra_width, iwd->extra_height, true_for_max);
		}
		else
			ts.width = ts.height = 0;
		return true;
	}

	void window_enabled(window wd, bool enabled)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd) && (iwd->flags.enabled != enabled))
		{
			iwd->flags.enabled = enabled;
			restrict::wd_manager().update(iwd, true, true);
			if(category::flags::root == iwd->other.category)
				interface_type::enable_window(iwd->root, enabled);
		}
	}

	bool window_enabled(window wd)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		return (restrict::wd_manager().available(iwd) ? iwd->flags.enabled : false);
	}

	//refresh_window
	//@brief: Refresh the window and display it immediately.
	void refresh_window(window wd)
	{
		restrict::wd_manager().update(reinterpret_cast<basic_window*>(wd), true, false);
	}

	void refresh_window_tree(window wd)
	{
		restrict::wd_manager().refresh_tree(reinterpret_cast<basic_window*>(wd));
	}

	//update_window
	//@brief: it displays a window immediately without refreshing.
	void update_window(window wd)
	{
		restrict::wd_manager().update(reinterpret_cast<basic_window*>(wd), false, true);
	}

	void window_caption(window wd, const std::string& title_utf8)
	{
		throw_not_utf8(title_utf8);
		auto const iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd))
			iwd->widget_notifier->caption(to_nstring(title_utf8));
	}

	void window_caption(window wd, const std::wstring& title)
	{
		auto const iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd))
			iwd->widget_notifier->caption(to_nstring(title));
	}

	std::string window_caption(window wd)
	{
		auto const iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd))
			return to_utf8(iwd->widget_notifier->caption());

		return{};
	}

	void window_cursor(window wd, cursor cur)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd))
		{
			iwd->predef_cursor = cur;
			restrict::bedrock.update_cursor(iwd);
		}
	}

	cursor window_cursor(window wd)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd))
			return iwd->predef_cursor;

		return cursor::arrow;
	}

	bool is_focus_ready(window wd)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd))
			return (iwd->root_widget->other.attribute.root->focus == iwd);

		return false;
	}

	void activate_window(window wd)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd))
		{
			if(iwd->flags.take_active)
				interface_type::activate_window(iwd->root);
		}
	}

	window focus_window()
	{
		internal_scope_guard lock;
		return reinterpret_cast<window>(restrict::bedrock.focus());
	}

	void focus_window(window wd)
	{
		restrict::wd_manager().set_focus(reinterpret_cast<basic_window*>(wd), false, arg_focus::reason::general);
		restrict::wd_manager().update(reinterpret_cast<basic_window*>(wd), false, false);
	}

	window capture_window()
	{
		return reinterpret_cast<window>(restrict::wd_manager().capture_window());
	}

	void set_capture(window wd, bool ignore_children)
	{
		restrict::wd_manager().capture_window(reinterpret_cast<basic_window*>(wd), true, ignore_children);
	}

	void release_capture(window wd)
	{
		//The 3rd parameter is useless when the 2nd parameter is false.
		restrict::wd_manager().capture_window(reinterpret_cast<basic_window*>(wd), false, false);
	}

	void modal_window(window wd)
	{
		auto const iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard isg;

		if (!restrict::wd_manager().available(iwd))
			return;

		if ((iwd->other.category == category::flags::root) && (iwd->flags.modal == false))
		{
			iwd->flags.modal = true;
#if defined(NANA_X11)
			interface_type::set_modal(iwd->root);
#endif
			restrict::wd_manager().show(iwd, true);
		}
		else
			return;

		//modal has to guarantee that does not lock the mutex of window_manager before invokeing the pump_event,
		//otherwise, the modal will prevent the other thread access the window.
		restrict::bedrock.pump_event(wd, true);
	}

	void wait_for(window wd)
	{
		internal_scope_guard lock;
		if (restrict::wd_manager().available(reinterpret_cast<basic_window*>(wd)))
			restrict::bedrock.pump_event(wd, false);
	}

	color fgcolor(window wd)
	{
		internal_scope_guard lock;
		if (restrict::wd_manager().available(reinterpret_cast<basic_window*>(wd)))
			return reinterpret_cast<basic_window*>(wd)->annex.scheme->foreground.get_color();
		return{};
	}

	color fgcolor(window wd, const color& clr)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd))
		{
			auto prev = iwd->annex.scheme->foreground.get_color();
			if (prev != clr)
			{
				iwd->annex.scheme->foreground = clr;
				restrict::wd_manager().update(iwd, true, false);
			}
			return prev;
		}
		return{};
	}

	color bgcolor(window wd)
	{
		internal_scope_guard lock;
		if (restrict::wd_manager().available(reinterpret_cast<basic_window*>(wd)))
			return reinterpret_cast<basic_window*>(wd)->annex.scheme->background.get_color();
		return{};
	}

	color bgcolor(window wd, const color& clr)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd))
		{
			auto prev = iwd->annex.scheme->background.get_color();
			if (prev != clr)
			{
				iwd->annex.scheme->background = clr;

				//If the bground mode of this window is basic, it should remake the background
				if (iwd->effect.bground && iwd->effect.bground_fade_rate < 0.01) // fade rate < 0.01 means it is basic mode
					iwd->flags.make_bground_declared = true;

				restrict::wd_manager().update(iwd, true, false);
			}
			return prev;
		}
		return{};
	}

	color activated_color(window wd)
	{
		internal_scope_guard lock;
		if (restrict::wd_manager().available(reinterpret_cast<basic_window*>(wd)))
			return reinterpret_cast<basic_window*>(wd)->annex.scheme->activated.get_color();
		return{};
	}

	color activated_color(window wd, const color& clr)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd))
		{
			auto prev = iwd->annex.scheme->activated.get_color();
			if (prev != clr)
			{
				iwd->annex.scheme->activated = clr;
				restrict::wd_manager().update(iwd, true, false);
			}
			return prev;
		}

		return{};
	}

	class caret_proxy
		: public caret_interface
	{
	public:
		caret_proxy(basic_window* wd)
			: window_{ wd }
		{}

		void disable_throw() noexcept override
		{
			throw_ = false;
		}

		void effective_range(const rectangle& range) override
		{
			internal_scope_guard lock;
			auto caret = _m_caret();
			if (caret)
				caret->effective_range(range);
		}

		void position(const point& pos) override
		{
			internal_scope_guard lock;
			auto caret = _m_caret();
			if (caret)
				caret->position(pos);
		}

		point position() const override
		{
			internal_scope_guard lock;
			auto caret = _m_caret();
			if (caret)
				return caret->position();

			return{};
		}

		void dimension(const size& size) override
		{
			internal_scope_guard lock;
			auto caret = _m_caret();
			if (caret)
				caret->dimension(size);
		}

		size dimension() const override
		{
			internal_scope_guard lock;
			auto caret = _m_caret();
			if (caret)
				return caret->dimension();

			return{};
		}

		void visible(bool visibility) override
		{
			internal_scope_guard lock;
			auto caret = _m_caret();
			if (caret)
				caret->visible(visibility);
		}

		bool visible() const override
		{
			internal_scope_guard lock;
			auto caret = _m_caret();
			return (caret && caret->visible());
		}

		bool activated() const override
		{
			internal_scope_guard lock;
			auto caret = _m_caret();
			return (caret && caret->activated());
		}
	private:
		caret_interface* _m_caret() const
		{
			if (restrict::wd_manager().available(window_) && window_->annex.caret_ptr)
				return window_->annex.caret_ptr;

			if (throw_)
				throw std::runtime_error("nana.api: access invalid caret");

			return nullptr;
		}
	private:
		basic_window* const window_;
		bool throw_{ true };
	};

	void create_caret(window wd, const size& caret_size)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd) && !(iwd->annex.caret_ptr))
			iwd->annex.caret_ptr = new ::nana::detail::caret(iwd, caret_size);
	}

	void destroy_caret(window wd)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd))
		{
			auto p = iwd->annex.caret_ptr;
			iwd->annex.caret_ptr = nullptr;
			delete p;
		}
	}

	std::unique_ptr<caret_interface> open_caret(window window_handle, bool disable_throw)
	{		
		auto p = new caret_proxy{ reinterpret_cast<basic_window*>(window_handle) };
		if (disable_throw)
			p->disable_throw();

		return std::unique_ptr<caret_interface>{ p };
	}

	void tabstop(window wd)
	{
		restrict::wd_manager().enable_tabstop(reinterpret_cast<basic_window*>(wd));
	}

	//eat_tabstop
	//@brief: set a eating tab window that it processes a pressing of tab itself
	void eat_tabstop(window wd, bool eat)
	{
		if(wd)
		{
			auto iwd = reinterpret_cast<basic_window*>(wd);
			internal_scope_guard isg;
			if(restrict::wd_manager().available(iwd))
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
		basic_window* ts_wd = restrict::wd_manager().tabstop(reinterpret_cast<basic_window*>(wd), next);
		restrict::wd_manager().set_focus(ts_wd, false, arg_focus::reason::general);
		restrict::wd_manager().update(ts_wd, false, false);
		return reinterpret_cast<window>(ts_wd);
	}

	void take_active(window wd, bool active, window take_if_active_false)
	{
		auto const iwd = reinterpret_cast<basic_window*>(wd);
		auto take_if_false = reinterpret_cast<basic_window*>(take_if_active_false);

		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd))
		{
			if (active || (take_if_false && (restrict::wd_manager().available(take_if_false) == false)))
				take_if_false = 0;

			iwd->flags.take_active = active;
			iwd->other.active_window = take_if_false;
		}
	}

	bool window_graphics(window wd, nana::paint::graphics& graph)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);

		internal_scope_guard lock;
		if (!restrict::wd_manager().available(iwd))
			return false;

		graph.make(iwd->drawer.graphics.size());
		graph.bitblt(0, 0, iwd->drawer.graphics);
		nana::detail::window_layout::paste_children_to_graphics(iwd, graph);
		return true;
	}

	bool root_graphics(window wd, nana::paint::graphics& graph)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd))
		{
			graph = *(iwd->root_graph);
			return true;
		}
		return false;
	}

	bool get_visual_rectangle(window wd, nana::rectangle& r)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd))
			return nana::detail::window_layout::read_visual_rectangle(iwd, r);
		
		return false;
	}

	void typeface(window wd, const nana::paint::font& font)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd))
		{
			iwd->drawer.graphics.typeface(font);
			iwd->drawer.typeface_changed();
			restrict::wd_manager().update(iwd, true, false);
		}
	}

	nana::paint::font typeface(window wd)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd))
			return iwd->drawer.graphics.typeface();

		return{};
	}

	bool calc_screen_point(window wd, nana::point& pos)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd))
		{
			pos += iwd->pos_root;
			return interface_type::calc_screen_point(iwd->root, pos);
		}
		return false;
	}

	bool calc_window_point(window wd, nana::point& pos)
	{
		return restrict::wd_manager().calc_window_point(reinterpret_cast<basic_window*>(wd), pos);
	}

	window find_window(const nana::point& pos)
	{
		auto wd = interface_type::find_window(pos.x, pos.y);
		if(wd)
		{
			::nana::point clipos{pos};
			interface_type::calc_window_point(wd, clipos);
			return reinterpret_cast<window>(
						restrict::wd_manager().find_window(wd, clipos));
		}
		return nullptr;
	}

	bool is_window_zoomed(window wd, bool ask_for_max)
	{
		auto const iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd))
		{
			if (iwd->other.category == nana::category::flags::root)
				return interface_type::is_window_zoomed(iwd->root, ask_for_max);
		}
		return false;
	}

	void widget_borderless(window wd, bool enabled)
	{
		auto const iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd))
		{
			if ((category::flags::widget == iwd->other.category) && (iwd->flags.borderless != enabled))
			{
				iwd->flags.borderless = enabled;
				restrict::wd_manager().update(iwd, true, false);
			}
		}
	}

	bool widget_borderless(window wd)
	{
		auto const iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd))
			return iwd->flags.borderless;

		return false;
	}

	nana::mouse_action mouse_action(window wd)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd))
			return iwd->flags.action;
		return nana::mouse_action::normal;
	}

	nana::element_state element_state(window wd)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if(restrict::wd_manager().available(iwd))
		{
			const bool is_focused = (iwd->root_widget->other.attribute.root->focus == iwd);
			switch(iwd->flags.action)
			{
			case nana::mouse_action::normal:
			case nana::mouse_action::normal_captured:
				return (is_focused ? nana::element_state::focus_normal : nana::element_state::normal);
			case nana::mouse_action::hovered:
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

	bool ignore_mouse_focus(window wd, bool ignore)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		if (restrict::wd_manager().available(iwd))
		{
			auto state = iwd->flags.ignore_mouse_focus;
			iwd->flags.ignore_mouse_focus = ignore;
			return state;
		}
		return false;
	}

	bool ignore_mouse_focus(window wd)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;
		return (restrict::wd_manager().available(iwd) ? iwd->flags.ignore_mouse_focus : false);
	}

	void at_safe_place(window wd, std::function<void()> fn)
	{
		restrict::wd_manager().set_safe_place(reinterpret_cast<basic_window*>(wd), std::move(fn));
	}

	std::optional<std::pair<size, size>> content_extent(window wd, unsigned limited_px, bool limit_width)
	{
		auto iwd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;

		if (restrict::wd_manager().available(iwd) && iwd->annex.content_measurer)
		{
			paint::graphics* graph = &iwd->drawer.graphics;
			paint::graphics temp_graph;
			if (graph->empty())
			{
				temp_graph.make({ 1, 1 });
				temp_graph.typeface(graph->typeface());
				graph = &temp_graph;
			}

			auto extent = iwd->annex.content_measurer->measure(*graph, limited_px, limit_width);
			if (extent)
				return std::make_pair(extent.value(), extent.value() + iwd->annex.content_measurer->extension());
		}
		
		return{};
	}

	unsigned screen_dpi(bool x_requested)
	{
		return ::nana::platform_abstraction::screen_dpi(x_requested);
	}

	dragdrop_status window_dragdrop_status(::nana::window wd)
	{
		auto real_wd = reinterpret_cast<basic_window*>(wd);
		internal_scope_guard lock;

		if (restrict::wd_manager().available(real_wd))
			return real_wd->other.dnd_state;

		return dragdrop_status::not_ready;
	}
}//end namespace API
}//end namespace nana
