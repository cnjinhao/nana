/*
 *	A Drawer Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
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
#include <nana/paint/image.hpp>
#include <functional>

namespace nana
{
	class widget;

	class drawer_trigger
		: ::nana::noncopyable, ::nana::nonmovable
	{
	public:
		typedef widget&		widget_reference;
		typedef paint::graphics&	graph_reference;

		virtual ~drawer_trigger();
		virtual void attached(widget_reference, graph_reference);	//none-const
		virtual void detached();	//none-const

		virtual void typeface_changed(graph_reference);
		virtual void refresh(graph_reference);

		virtual void resizing(graph_reference, const arg_resizing&);
		virtual void resized(graph_reference, const arg_resized&);
		virtual void move(graph_reference, const arg_move&);
		virtual void click(graph_reference, const arg_mouse&);
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

		void _m_reset_overrided();
		bool _m_overrided() const;
	private:
		bool overrided_{false};
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
				unknown,
				overrided,
				not_overrided
			};
		public:
			~drawer();

			void bind(basic_window*);

			void typeface_changed();
			void click(const arg_mouse&);
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
			void map(window);	//Copy the root buffer to screen
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
			void _m_use_refresh();

			template<typename Arg, typename Mfptr>
			void _m_emit(event_code evt_code, const Arg& arg, Mfptr mfptr)
			{
				if (realizer_)
				{
					const int pos = static_cast<int>(evt_code);
					if (method_state::not_overrided != mth_state_[pos])
					{
						_m_bground_pre();

						if (method_state::unknown == mth_state_[pos])
						{
							realizer_->_m_reset_overrided();
							(realizer_->*mfptr)(graphics, arg);
							mth_state_[pos] = (realizer_->_m_overrided() ? method_state::overrided : method_state::not_overrided);
						}
						else
							(realizer_->*mfptr)(graphics, arg);

						_m_use_refresh();
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
