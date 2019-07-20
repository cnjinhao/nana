#ifndef NANA_GUI_DETAIL_BEDROCK_TYPES_INCLUDED
#define NANA_GUI_DETAIL_BEDROCK_TYPES_INCLUDED

#include <nana/push_ignore_diagnostic>

#include <nana/gui/detail/bedrock.hpp>
#include <nana/gui/detail/color_schemes.hpp>
#include <nana/gui/detail/events_operation.hpp>
#include <nana/gui/detail/window_manager.hpp>
#include <set>

namespace nana
{
	namespace detail
	{
		struct bedrock::pi_data
		{
			color_schemes				scheme;
			events_operation			evt_operation;
			window_manager				wd_manager;
			std::set<basic_window*>	auto_form_set;
			bool shortkey_occurred{ false };

			struct menu_rep
			{
				basic_window*	taken_window{ nullptr };
				bool			delay_restore{ false };
				native_window_type window{ nullptr };
				native_window_type owner{ nullptr };
				bool	has_keyboard{ false };
			}menu;
		};


#ifdef NANA_WINDOWS
		struct bedrock::thread_context
		{
			unsigned	event_pump_ref_count{0};
			int			window_count{0};	//The number of windows
			basic_window* event_window{nullptr};

			struct platform_detail_tag
			{
				wchar_t keychar;
			}platform;

			struct cursor_tag
			{
				basic_window * window;
				native_window_type native_handle;
				nana::cursor	predef_cursor;
				HCURSOR			handle;
			}cursor;

			thread_context()
			{
				cursor.window = nullptr;
				cursor.native_handle = nullptr;
				cursor.predef_cursor = nana::cursor::arrow;
				cursor.handle = nullptr;
			}
		};
#else
		struct bedrock::thread_context
		{
			unsigned event_pump_ref_count{0};

			int		window_count{0};	//The number of windows
			basic_window* event_window{nullptr};
			bool	is_alt_pressed{false};
			bool	is_ctrl_pressed{false};

			struct platform_detail_tag
			{
				native_window_type	motion_window;
				nana::point		motion_pointer_pos;
			}platform;

			struct cursor_tag
			{
				basic_window * window;
				native_window_type native_handle;
				nana::cursor predef_cursor;
				Cursor handle;
			}cursor;

			thread_context()
			{
				cursor.window = nullptr;
				cursor.native_handle = nullptr;
				cursor.predef_cursor = nana::cursor::arrow;
				cursor.handle = 0;
			}
		};
#endif
	}
}

#include <nana/pop_ignore_diagnostic>

#endif