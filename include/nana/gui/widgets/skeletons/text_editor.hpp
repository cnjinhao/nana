/*
 *	A text editor implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
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
#include "textbase.hpp"
#include "text_editor_part.hpp"
#include <nana/gui/widgets/scroll.hpp>
#include <nana/unicode_bidi.hpp>

namespace nana{	namespace widgets
{
	namespace skeletons
	{
		template<typename EnumCommand>
		class undoable_command_interface
		{
		public:
			virtual ~undoable_command_interface() = default;

			virtual EnumCommand get() const = 0;
			virtual bool merge(const undoable_command_interface&) = 0;
			virtual void execute(bool redo) = 0;
		};

		template<typename EnumCommand>
		class undoable
		{
		public:
			using command = EnumCommand;
			using container = std::deque < std::unique_ptr<undoable_command_interface<command>> >;

			void max_steps(std::size_t maxs)
			{
				max_steps_ = maxs;
				if (maxs && (commands_.size() >= maxs))
					commands_.erase(commands_.begin(), commands_.begin() + (commands_.size() - maxs + 1));
			}

			std::size_t max_steps() const
			{
				return max_steps_;
			}

			void enable(bool enb)
			{
				enabled_ = enb;
				if (!enb)
					commands_.clear();
			}

			bool enabled() const
			{
				return enabled_;
			}

			void push(std::unique_ptr<undoable_command_interface<command>> && ptr)
			{
				if (!ptr || !enabled_)
					return;

				if (pos_ < commands_.size())
					commands_.erase(commands_.begin() + pos_, commands_.end());
				else if (max_steps_ && (commands_.size() >= max_steps_))
					commands_.erase(commands_.begin(), commands_.begin() + (commands_.size() - max_steps_ + 1));

				pos_ = commands_.size();
				if (!commands_.empty())
				{
					if (commands_.back().get()->merge(*ptr))
						return;
				}

				commands_.emplace_back(std::move(ptr));
				++pos_;
			}

			std::size_t count(bool is_undo) const
			{
				return (is_undo ? pos_ : commands_.size() - pos_);
			}

			void undo()
			{
				if (pos_ > 0)
				{
					--pos_;
					commands_[pos_].get()->execute(false);
				}
			}

			void redo()
			{
				if (pos_ != commands_.size())
					commands_[pos_++].get()->execute(true);
			}

		private:
			container commands_;
			bool		enabled_{ true };
			std::size_t max_steps_{ 30 };
			std::size_t pos_{ 0 };
		};

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

			struct keywords;
			class keyword_parser;
		public:
			using char_type = wchar_t;
			using size_type = textbase<char_type>::size_type;
			using string_type = textbase<char_type>::string_type;

			using event_interface = text_editor_event_interface;

			using graph_reference = ::nana::paint::graphics&;

			struct ext_renderer_tag
			{
				std::function<void(graph_reference, const nana::rectangle& text_area, const ::nana::color&)> background;
			};

			enum class accepts
			{
				no_restrict, integer, real
			};

			text_editor(window, graph_reference, const text_editor_scheme*);
			~text_editor();

			void set_highlight(const ::std::string& name, const ::nana::color&, const ::nana::color&);
			void erase_highlight(const ::std::string& name);
			void set_keyword(const ::std::wstring& kw, const std::string& name, bool case_sensitive, bool whole_word_matched);
			void erase_keyword(const ::std::wstring& kw);

			void set_accept(std::function<bool(char_type)>);
			void set_accept(accepts);
			bool respond_char(const arg_keyboard& arg);
			bool respond_key(const arg_keyboard& arg);

			void typeface_changed();

			void indent(bool, std::function<std::string()> generator);
			void set_event(event_interface*);

			/// Determine whether the text_editor is line wrapped.
			bool line_wrapped() const;
			/// Set the text_editor whether it is line wrapped, it returns false if the state is not changed.
			bool line_wrapped(bool);

			void border_renderer(std::function<void(graph_reference, const ::nana::color& bgcolor)>);

			bool load(const char*);

			/// Sets the text area.
			/// @return true if the area is changed with the new value.
			bool text_area(const nana::rectangle&);

			/// Returns the text area
			rectangle text_area(bool including_scroll) const;

			bool tip_string(::std::string&&);

			const attributes & attr() const;
			bool multi_lines(bool);
			void editable(bool);
			void enable_background(bool);
			void enable_background_counterpart(bool);

			void undo_enabled(bool);
			bool undo_enabled() const;
			void undo_max_steps(std::size_t);
			std::size_t undo_max_steps() const;

			ext_renderer_tag& ext_renderer() const;

			unsigned line_height() const;
			unsigned screen_lines() const;

			bool getline(std::size_t pos, ::std::wstring&) const;
			void text(std::wstring);
			std::wstring text() const;

			/// Sets caret position through text coordinate.
			void move_caret(const upoint&);
			void move_caret_end();
			void reset_caret_pixels() const;
			void reset_caret();
			void show_caret(bool isshow);

			bool selected() const;
			bool select(bool);
			/// Sets the end position of a selected string.
			void set_end_caret();
			
			bool hit_text_area(const point&) const;
			bool hit_select_area(nana::upoint pos) const;

			bool move_select();
			bool mask(wchar_t);

			/// Returns width of text area excluding the vscroll size.
			unsigned width_pixels() const;
			window window_handle() const;

			/// Returns text position of each line that currently displays on screen
			const std::vector<upoint>& text_position() const;
		public:
			void draw_corner();
			void render(bool focused);
		public:
			void put(std::wstring);
			void put(wchar_t);
			void copy() const;
			void cut();
			void paste();
			void enter(bool record_undo = true);
			void del();
			void backspace(bool record_undo = true);
			void undo(bool reverse);
			void move_ns(bool to_north);	//Moves up and down
			void move_left();
			void move_right();
			const upoint& mouse_caret(const point& screen_pos);
			const upoint& caret() const;
			point caret_screen_pos() const;
			bool scroll(bool upwards, bool vertical);
			bool mouse_enter(bool);
			bool mouse_move(bool left_button, const point& screen_pos);
			bool mouse_pressed(const arg_mouse& arg);

			skeletons::textbase<wchar_t>& textbase();
			const skeletons::textbase<wchar_t>& textbase() const;
		private:
			bool _m_accepts(char_type) const;
			::nana::color _m_bgcolor() const;
			bool _m_scroll_text(bool vertical);
			void _m_on_scroll(const arg_mouse&);
			void _m_scrollbar();
			::nana::size _m_text_area() const;
			void _m_get_scrollbar_size();
			void _m_reset();
			::nana::upoint _m_put(::std::wstring);
			::nana::upoint _m_erase_select();

			bool _m_make_select_string(::std::wstring&) const;
			static bool _m_resolve_text(const ::std::wstring&, std::vector<std::pair<std::size_t, std::size_t>> & lines);

			bool _m_cancel_select(int align);
			unsigned _m_tabs_pixels(size_type tabs) const;
			nana::size _m_text_extent_size(const char_type*, size_type n) const;

			/// Moves the view of window.
			bool _m_move_offset_x_while_over_border(int many);
			bool _m_move_select(bool record_undo);

			int _m_text_top_base() const;

			/// Returns the right/bottom point of text area.
			int _m_end_pos(bool right) const;	

			void _m_draw_parse_string(const keyword_parser&, bool rtl, ::nana::point pos, const ::nana::color& fgcolor, const wchar_t*, std::size_t len) const;
			//_m_draw_string
			//@brief: Draw a line of string
			void _m_draw_string(int top, const ::nana::color&, const nana::upoint& str_pos, const ::std::wstring&, bool if_mask) const;
			//_m_update_caret_line
			//@brief: redraw whole line specified by caret pos. 
			//@return: true if caret overs the border
			bool _m_update_caret_line(std::size_t secondary_before);
			bool _m_get_sort_select_points(nana::upoint&, nana::upoint&) const;

			void _m_offset_y(int y);

			unsigned _m_char_by_pixels(const wchar_t*, std::size_t len, unsigned* pxbuf, int str_px, int pixels, bool is_rtl);
			unsigned _m_pixels_by_char(const ::std::wstring&, std::size_t pos) const;
			void _handle_move_key(const arg_keyboard& arg);

		private:
			std::unique_ptr<editor_behavior_interface> behavior_;
			undoable<command>	undo_;
			nana::window window_;
			graph_reference graph_;
			const text_editor_scheme*	scheme_;
			event_interface *			event_handler_{ nullptr };
			std::unique_ptr<keywords> keywords_;

			skeletons::textbase<wchar_t> textbase_;
			wchar_t mask_char_{0};

			mutable ext_renderer_tag ext_renderer_;

			std::vector<upoint> text_position_;	//position of text from last rendering.

			struct indent_rep
			{
				bool enabled{ false };
				std::function<std::string()> generator;
			}indent_;

			struct attributes
			{
				accepts acceptive{ accepts::no_restrict };
				std::function<bool(char_type)> pred_acceptive;

				::std::string tip_string;

				bool line_wrapped{false};
				bool multi_lines{true};
				bool editable{true};
				bool enable_background{true};
				bool enable_counterpart{false};
				nana::paint::graphics counterpart; //this is used to keep the background that painted by external part.

				std::unique_ptr<nana::scroll<true>>		vscroll;
				std::unique_ptr<nana::scroll<false>>	hscroll;
			}attributes_;

			struct text_area_type
			{
				nana::rectangle	area;

				bool		captured;
				unsigned	tab_space;
				unsigned	scroll_pixels;
				unsigned	vscroll;
				unsigned	hscroll;
				std::function<void(nana::paint::graphics&, const ::nana::color&)> border_renderer;
			}text_area_;

			struct selection
			{
				enum mode_selection_t{mode_no_selected, mode_mouse_selected, mode_method_selected};

				mode_selection_t mode_selection;
				bool dragged;
				nana::upoint a, b;
			}select_;

			struct coordinate
			{
				nana::point		offset;	//x stands for pixels, y for lines
				nana::upoint	caret;	//position of caret by text, it specifies the position of a new character
				nana::upoint	shift_begin_caret;
				unsigned		xpos{0};	//This data is used for move up/down
			}points_;
		};
	}//end namespace skeletons
}//end namespace widgets
}//end namespace nana

#endif

