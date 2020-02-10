/*
 *	A Drawer Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2017 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/drawer.hpp
 */

#ifndef NANA_GUI_DETAIL_DRAWER_HPP
#define NANA_GUI_DETAIL_DRAWER_HPP

#include <nana/push_ignore_diagnostic>
#include "general_events.hpp"
#include <nana/paint/graphics.hpp>
#include <functional>
#include <vector>

namespace nana
{
	class widget;

	namespace detail
	{
		class drawer;
	}

	class drawer_trigger;
	class event_filter_status
	{
	public:
		event_filter_status();
		event_filter_status(const event_filter_status& rOther);
		event_filter_status(const unsigned evt_disabled_);
		const event_filter_status& operator=(const event_filter_status& rOther);
		const event_filter_status& operator=(const unsigned evt_disabled_);

		bool operator[](const nana::event_code evt_code) const;
		bool operator==(const event_filter_status& rOther) const;
		bool operator!=(const event_filter_status& rOther) const;

	private:
		unsigned evt_disabled_;
		friend class drawer_trigger;
	};

	class drawer_trigger
	{
		friend class detail::drawer;

		//Noncopyable
		drawer_trigger(const drawer_trigger&) = delete;
		drawer_trigger& operator=(const drawer_trigger&) = delete;

		//Nonmovable
		drawer_trigger(drawer_trigger&&) = delete;
		drawer_trigger& operator=(drawer_trigger&&) = delete;

	public:
		using widget_reference = widget&;
		using graph_reference = paint::graphics&;

		drawer_trigger() = default;
		virtual ~drawer_trigger() = default;
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
		virtual void key_ime(graph_reference, const arg_ime&);
		virtual void key_press(graph_reference, const arg_keyboard&);
		virtual void key_char(graph_reference, const arg_keyboard&);
		virtual void key_release(graph_reference, const arg_keyboard&);
		virtual void shortkey(graph_reference, const arg_keyboard&);

		void filter_event(const event_code evt_code, const bool bDisabled);
		void filter_event(const std::vector<event_code>& evt_codes, const bool bDisabled);
		void filter_event(const event_filter_status& evt_all_states);
		bool filter_event(const event_code evt_code);
		event_filter_status filter_event();
		void clear_filter();

	private:
		void _m_reset_overridden();
		bool _m_overridden(event_code) const;
	private:
		unsigned overridden_{ 0xFFFFFFFF };
		unsigned evt_disabled_{ 0 }; // bit set if event is filtered
	};

	namespace detail
	{
		struct basic_window;

		//@brief:	Every window has a drawer, the drawer holds a drawer_trigger for
		//			a widget.
		class drawer
			: nana::noncopyable, nana::nonmovable
		{
			enum class method_state
			{
				pending,
				overridden,
				not_overridden
			};
		public:
			drawer();
			~drawer();

			void bind(basic_window*);

			void typeface_changed();
			void click(const arg_click&, const bool);
			void dbl_click(const arg_mouse&, const bool);
			void mouse_enter(const arg_mouse&, const bool);
			void mouse_move(const arg_mouse&, const bool);
			void mouse_leave(const arg_mouse&, const bool);
			void mouse_down(const arg_mouse&, const bool);
			void mouse_up(const arg_mouse&, const bool);
			void mouse_wheel(const arg_wheel&, const bool);
			void mouse_dropfiles(const arg_dropfiles&, const bool);
			void resizing(const arg_resizing&, const bool);
			void resized(const arg_resized&, const bool);
			void move(const arg_move&, const bool);
			void focus(const arg_focus&, const bool);
			void key_ime(const arg_ime& arg, const bool bForce__EmitInternal);
			void key_press(const arg_keyboard&, const bool);
			void key_char(const arg_keyboard&, const bool);
			void key_release(const arg_keyboard&, const bool);
			void shortkey(const arg_keyboard&, const bool);
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
			void _m_effect_bground_subsequent();
			method_state& _m_mth_state(int pos);

			template<typename Arg, typename Mfptr>
			void _m_emit(event_code evt_code, const Arg& arg, Mfptr mfptr, const bool bForce__EmitInternal)
			{
				const int pos = static_cast<int>(evt_code);

				auto realizer = this->realizer();
				auto & mth_state = _m_mth_state(pos);

				if (realizer && (method_state::not_overridden != mth_state))
				{
					const bool bFiltered = !bForce__EmitInternal && realizer->filter_event(evt_code);
					if (method_state::pending == mth_state)
					{
						if (!bFiltered)
							(realizer->*mfptr)(graphics, arg);

						//Check realizer, when the window is closed in that event handler, the drawer will be
						//detached and realizer will be a nullptr
						if (realizer)
							mth_state = (realizer->_m_overridden(evt_code) ? method_state::overridden : method_state::not_overridden);
					}
					else
					{
						if (!bFiltered)
							(realizer->*mfptr)(graphics, arg);
					}

					_m_effect_bground_subsequent();
				}
			}
		public:
			nana::paint::graphics graphics;
		private:
			struct data_implement;

			data_implement * const data_impl_;
		};
	}//end namespace detail
}//end namespace nana

#include <nana/pop_ignore_diagnostic>

#endif
