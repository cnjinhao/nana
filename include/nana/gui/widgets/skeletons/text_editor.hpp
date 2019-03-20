/*
 *	A text editor implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/skeletons/text_editor.hpp
 *	@description: 
 */

#ifndef NANA_GUI_SKELETONS_TEXT_EDITOR_HPP
#define NANA_GUI_SKELETONS_TEXT_EDITOR_HPP
#include <nana/push_ignore_diagnostic>

#include "textbase.hpp"
#include "text_editor_part.hpp"
#include <nana/unicode_bidi.hpp>

#include <nana/gui/detail/general_events.hpp>

#include <functional>

namespace nana
{
	namespace paint
	{
		// Forward declaration
		class graphics;
	}
}

namespace nana{	namespace widgets
{
	namespace skeletons
	{
		class text_editor
		{
			struct attributes;
			class editor_behavior_interface;
			class behavior_normal;
			class behavior_linewrapped;

			enum class command{
				backspace, input_text, move_text,
			};
			//Commands for undoable
			template<typename EnumCommand> class basic_undoable;
			class undo_backspace;
			class undo_input_text;
			class undo_move_text;

			class keyword_parser;
			class helper_pencil;

			struct text_section;

			text_editor(const text_editor&) = delete;
			text_editor& operator=(const text_editor&) = delete;

			text_editor(text_editor&&) = delete;
			text_editor& operator=(text_editor&&) = delete;
		public:
			using char_type = wchar_t;
			using size_type = textbase<char_type>::size_type;
			using string_type = textbase<char_type>::string_type;
			using path_type = std::filesystem::path;

			using event_interface = text_editor_event_interface;

			using graph_reference = ::nana::paint::graphics&;

			struct renderers
			{
				std::function<void(graph_reference, const nana::rectangle& text_area, const ::nana::color&)> background;	///< a customized background renderer
				std::function<void(graph_reference, const ::nana::color&)> border;											///< a customized border renderer
			};

			enum class accepts
			{
				no_restrict, integer, real
			};

			text_editor(window, graph_reference, const text_editor_scheme*);
			~text_editor();

			size caret_size() const;
			const point& content_origin() const;

			void set_highlight(const ::std::string& name, const ::nana::color&, const ::nana::color&);
			void erase_highlight(const ::std::string& name);
			void set_keyword(const ::std::wstring& kw, const std::string& name, bool case_sensitive, bool whole_word_matched);
			void erase_keyword(const ::std::wstring& kw);

			colored_area_access_interface& colored_area();

			void set_accept(std::function<bool(char_type)>);
			void set_accept(accepts);
			bool respond_char(const arg_keyboard& arg);
			bool respond_key(const arg_keyboard& arg);

			void typeface_changed();

			void indent(bool, std::function<std::string()> generator);
			void set_event(event_interface*);

			bool load(const path_type& file);

			void text_align(::nana::align alignment);

			/// Sets the text area.
			/// @return true if the area is changed with the new value.
			bool text_area(const nana::rectangle&);

			/// Returns the text area
			rectangle text_area(bool including_scroll) const;

			bool tip_string(::std::string&&);

			/// Returns the reference of listbox attributes
			const attributes & attr() const noexcept;

			/// Set the text_editor whether it is line wrapped, it returns false if the state is not changed.
			bool line_wrapped(bool);

			bool multi_lines(bool);

			/// Enables/disables the editability of text_editor
			/**
			 * @param enable Indicates whether to enable or disable the editability
			 * @param enable_cart Indicates whether to show or hide the caret when the text_editor is not editable. It is ignored if enable is false.
			 */
			void editable(bool enable, bool enable_caret);
			void enable_background(bool);
			void enable_background_counterpart(bool);

			void undo_clear();
			void undo_max_steps(std::size_t);
			std::size_t undo_max_steps() const;

			renderers& customized_renderers();

			unsigned line_height() const;
			unsigned screen_lines(bool completed_line = false) const;

			bool getline(std::size_t pos, ::std::wstring&) const;
			void text(std::wstring, bool end_caret);
			std::wstring text() const;

			/// Moves the caret at specified position
			/**
			 * @param pos the text position
			 * @param stay_in_view Indicates whether to adjust the view to make the caret in view. This parameter is ignored if the caret is already in view.
			 * @return true indicates a refresh is required.
			 */
			bool move_caret(upoint pos, bool stay_in_view = false);
			void move_caret_end(bool update);
			void reset_caret_pixels() const;
			void reset_caret(bool stay_in_view = false);
			void show_caret(bool isshow);

			bool selected() const;
			bool get_selected_points(nana::upoint&, nana::upoint&) const;

			bool select(bool);

			bool select_points(nana::upoint arg_a, nana::upoint arg_b);

			/// Sets the end position of a selected string.
			void set_end_caret(bool stay_in_view);
			
			bool hit_text_area(const point&) const;
			bool hit_select_area(nana::upoint pos, bool ignore_when_select_all) const;

			bool move_select();
			bool mask(wchar_t);

			/// Returns width of text area excluding the vscroll size.
			unsigned width_pixels() const;
			window window_handle() const;

			/// Returns text position of each line that currently displays on screen
			const std::vector<upoint>& text_position() const;

			void focus_behavior(text_focus_behavior);
			void select_behavior(bool move_to_end);

			std::size_t line_count(bool text_lines) const;
		public:
			void draw_corner();
			void render(bool focused);
		public:
			void put(std::wstring, bool perform_event);
			void put(wchar_t);
			void copy() const;
			void cut();
			void paste();
			void enter(bool record_undo, bool perform_event);
			void del();
			void backspace(bool record_undo, bool perform_event);
			void undo(bool reverse);
			void move_ns(bool to_north);	//Moves up and down
			void move_left();
			void move_right();
			const upoint& mouse_caret(const point& screen_pos, bool stay_in_view);
			const upoint& caret() const noexcept;
			point caret_screen_pos() const;
			bool scroll(bool upwards, bool vertical);

			bool focus_changed(const arg_focus&);
			bool mouse_enter(bool entering);
			bool mouse_move(bool left_button, const point& screen_pos);
			void mouse_pressed(const arg_mouse& arg);
			bool select_word(const arg_mouse& arg);

			skeletons::textbase<char_type>& textbase() noexcept;
			const skeletons::textbase<char_type>& textbase() const noexcept;

			bool try_refresh();

			std::shared_ptr<scroll_operation_interface> scroll_operation() const;
		private:
			nana::color _m_draw_colored_area(paint::graphics& graph, const std::pair<std::size_t,std::size_t>& row, bool whole_line);
			std::vector<upoint> _m_render_text(const ::nana::color& text_color);
			void _m_pre_calc_lines(std::size_t line_off, std::size_t lines);

			//Caret to screen coordinate or context coordiate(in pixels)
			::nana::point	_m_caret_to_coordinate(::nana::upoint pos, bool to_screen_coordinate = true) const;
			//Screen coordinate or context coordinate(in pixels) to caret,
			::nana::upoint	_m_coordinate_to_caret(::nana::point pos, bool from_screen_coordinate = true) const;

			bool _m_pos_from_secondary(std::size_t textline, const nana::upoint& secondary, unsigned & pos);
			bool _m_pos_secondary(const nana::upoint& charpos, nana::upoint& secondary_pos) const;
			bool _m_move_caret_ns(bool to_north);
			void _m_update_line(std::size_t pos, std::size_t secondary_count_before);

			bool _m_accepts(char_type) const;
			::nana::color _m_bgcolor() const;

			void _m_reset_content_size(bool calc_lines = false);
			void _m_reset();

			//Inserts text at position where the caret is
			::nana::upoint _m_put(::std::wstring, bool perform_event);

			::nana::upoint _m_erase_select(bool perform_event);

			::std::wstring _m_make_select_string() const;
			static bool _m_resolve_text(const ::std::wstring&, std::vector<std::pair<std::size_t, std::size_t>> & lines);

			bool _m_cancel_select(int align);
			nana::size _m_text_extent_size(const char_type*, size_type n) const;

			/// Adjust position of view to make caret stay in screen
			bool _m_adjust_view();

			bool _m_move_select(bool record_undo);

			int _m_text_top_base() const;
			int _m_text_topline() const;

			/// Returns the logical position that text starts of a specified line in x-axis
			int _m_text_x(const text_section&) const;

			void _m_draw_parse_string(const keyword_parser&, bool rtl, ::nana::point pos, const ::nana::color& fgcolor, const wchar_t*, std::size_t len) const;
			//_m_draw_string
			//@brief: Draw a line of string
			void _m_draw_string(int top, const ::nana::color&, const nana::upoint& str_pos, const text_section&, bool if_mask) const;
			//_m_update_caret_line
			//@brief: redraw whole line specified by caret pos. 
			//@return: true if caret overs the border
			bool _m_update_caret_line(std::size_t secondary_before);

			unsigned _m_char_by_pixels(const unicode_bidi::entity&, unsigned pos) const;
			unsigned _m_pixels_by_char(const ::std::wstring&, ::std::size_t pos) const;
			void _m_handle_move_key(const arg_keyboard& arg);

			unsigned _m_width_px(bool exclude_vs) const;
			void _m_draw_border();
		private:
			struct implementation;
			implementation * const impl_;

			nana::window window_;
			graph_reference graph_;
			const text_editor_scheme*	scheme_;
			event_interface *			event_handler_{ nullptr };

			wchar_t mask_char_{0};

			struct attributes
			{
				::std::string tip_string;

				::nana::align alignment{ ::nana::align::left };

				bool line_wrapped{false};
				bool multi_lines{true};
				bool editable{true};
				bool enable_caret{ true }; ///< Indicates whether to show or hide caret when text_editor is not editable
				bool enable_background{true};
			}attributes_;

			struct text_area_type
			{
				nana::rectangle	area;

				bool		captured{ false };
				unsigned	tab_space{ 4 };
			}text_area_;

			struct selection
			{
				enum class mode{ no_selected, mouse_selected, method_selected, move_selected, move_selected_take_effect };

				bool ignore_press{ false };
				bool move_to_end{ false };
				mode mode_selection{ mode::no_selected };
				text_focus_behavior behavior{text_focus_behavior::none};

				nana::upoint a, b;
			}select_;

			struct coordinate
			{
				nana::upoint	caret;	//position of caret by text, it specifies the position of a new character
				nana::upoint	shift_begin_caret;
			}points_;
		};
	}//end namespace skeletons
}//end namespace widgets
}//end namespace nana

#include <nana/pop_ignore_diagnostic>

#endif

