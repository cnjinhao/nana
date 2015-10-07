/*
 *	A Drawer Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/drawer.hpp
 */

#ifndef NANA_GUI_DETAIL_DRAWER_HPP
#define NANA_GUI_DETAIL_DRAWER_HPP

#include <vector>
#include "general_events.hpp"
#include <nana/paint/graphics.hpp>
#include <functional>

namespace nana
{
	class widget;

	namespace detail
	{
		class drawer;
	}

	class drawer_trigger
		: ::nana::noncopyable, ::nana::nonmovable
	{
		friend class detail::drawer;
	public:
		using widget_reference = widget&;
		using graph_reference = paint::graphics&;

		virtual ~drawer_trigger();
		virtual void attached(widget_reference, graph_reference);	//none-const
		virtual void detached();	//none-const

		virtual void typeface_changed(graph_reference);
		virtual void refresh(graph_reference);

		virtual void resizing(graph_reference, const arg_resizing&);
		virtual void resized(graph_reference, const arg_resized&);
		virtual void move(graph_reference, const arg_move&);
		virtual void click(graph_reference, const arg_click&);
		virtual void dbl_click(graph_reference, const arg_mouse&);
		virtual void mouse_enter(graph_reference, const arg_mouse&);
		virtual void mouse_move(graph_reference, const arg_mouse&);
		virtual void mouse_leave(graph_reference, const arg_mouse&);
		virtual void mouse_down(graph_reference, const arg_mouse&);
		virtual void mouse_up(graph_reference, const arg_mouse&);
		virtual void mouse_wheel(graph_reference, const arg_wheel&);
		virtual void mouse_dropfiles(graph_reference, const arg_dropfiles&);

		virtual void focus(graph_reference, const arg_focus&);
		virtual void key_press(graph_reference, const arg_keyboard&);
		virtual void key_char(graph_reference, const arg_keyboard&);
		virtual void key_release(graph_reference, const arg_keyboard&);
		virtual void shortkey(graph_reference, const arg_keyboard&);

	private:
		void _m_reset_overrided();
		bool _m_overrided(event_code) const;
	private:
		unsigned overrided_{ 0xFFFFFFFF };
	};

	namespace detail
	{
		struct basic_window;

		namespace dynamic_drawing
		{
			//declaration
			class object;
		}

		//@brief:	Every window has a drawer, the drawer holds a drawer_trigger for
		//			a widget.
		class drawer
			: nana::noncopyable, nana::nonmovable
		{
			enum{
				event_size = static_cast<int>(event_code::end)
			};

			enum class method_state
			{
				pending,
				overrided,
				not_overrided
			};
		public:
			~drawer();

			void bind(basic_window*);

			void typeface_changed();
			void click(const arg_click&);
			void dbl_click(const arg_mouse&);
			void mouse_enter(const arg_mouse&);
			void mouse_move(const arg_mouse&);
			void mouse_leave(const arg_mouse&);
			void mouse_down(const arg_mouse&);
			void mouse_up(const arg_mouse&);
			void mouse_wheel(const arg_wheel&);
			void mouse_dropfiles(const arg_dropfiles&);
			void resizing(const arg_resizing&);
			void resized(const arg_resized&);
			void move(const arg_move&);
			void focus(const arg_focus&);
			void key_press(const arg_keyboard&);
			void key_char(const arg_keyboard&);
			void key_release(const arg_keyboard&);
			void shortkey(const arg_keyboard&);
			void map(window, bool forced, const rectangle* update_area = nullptr);	//Copy the root buffer to screen
			void refresh();
			drawer_trigger* realizer() const;
			void attached(widget&, drawer_trigger&);
			drawer_trigger* detached();
		public:
			void clear();
			void* draw(std::function<void(paint::graphics&)> &&, bool diehard);
			void erase(void* diehard);
		private:
			void _m_bground_pre();
			void _m_bground_end();
			void _m_draw_dynamic_drawing_object();
			bool _m_lazy_decleared() const;

			template<typename Arg, typename Mfptr>
			void _m_emit(event_code evt_code, const Arg& arg, Mfptr mfptr)
			{
				const int pos = static_cast<int>(evt_code);
				if (realizer_ && (method_state::not_overrided != mth_state_[pos]))
				{
					_m_bground_pre();

					if (method_state::pending == mth_state_[pos])
					{
						(realizer_->*mfptr)(graphics, arg);
						
						//Check realizer, when the window is closed in that event handler, the drawer will be
						//detached and realizer will be a nullptr
						if(realizer_)
							mth_state_[pos] = (realizer_->_m_overrided(evt_code) ? method_state::overrided : method_state::not_overrided);
					}
					else
						(realizer_->*mfptr)(graphics, arg);

					if (_m_lazy_decleared())
					{
						_m_draw_dynamic_drawing_object();
						_m_bground_end();
					}
				}
			}
		public:
			nana::paint::graphics graphics;
		private:
			basic_window*	core_window_{nullptr};
			drawer_trigger*	realizer_{nullptr};
			std::vector<dynamic_drawing::object*>	dynamic_drawing_objects_;
			bool refreshing_{false};
			method_state mth_state_[event_size];
		};
	}//end namespace detail
}//end namespace nana

#endif
