/**
 *	A Textbox Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/textbox.hpp
 */
#ifndef NANA_GUI_WIDGET_TEXTBOX_HPP
#define NANA_GUI_WIDGET_TEXTBOX_HPP
#include <nana/push_ignore_diagnostic>

#include <nana/gui/widgets/widget.hpp>
#include "skeletons/textbase_export_interface.hpp"
#include "skeletons/text_editor_part.hpp"

#include <nana/optional.hpp>

namespace nana
{
	class textbox;

	struct arg_textbox
		: public event_arg
	{
		textbox& widget;
		const std::vector<upoint>& text_position;	///< position of characters that the first character of line which are displayed

		arg_textbox(textbox&, const std::vector<upoint>&);
	};

	namespace drawerbase
	{
		namespace textbox
		{
			struct textbox_events
				: public general_events
			{
				basic_event<arg_textbox> first_change;
				basic_event<arg_textbox> text_changed;
				basic_event<arg_textbox> text_exposed;
			};

			class event_agent
				:	public	widgets::skeletons::textbase_event_agent_interface,
					public	widgets::skeletons::text_editor_event_interface 
			{
			public:
				event_agent(::nana::textbox&, const std::vector<upoint>&);
			private:
				//Overrides textbase_event_agent_interface
				void first_change() override;
				void text_changed() override;
			private:
				//Overrides text_editor_event_interface
				void text_exposed(const std::vector<upoint>&) override;
			private:
				::nana::textbox & widget_;
				const std::vector<upoint>& text_position_;
			};

			//class drawer
			class drawer
				: public drawer_trigger
			{
			public:
				using text_editor = widgets::skeletons::text_editor;

				drawer();
				text_editor * editor();
				const text_editor * editor() const;
			private:
				void attached(widget_reference, graph_reference)	override;
				void detached()	override;
				void refresh(graph_reference)	override;
				void focus(graph_reference, const arg_focus&)	override;
				void mouse_down(graph_reference, const arg_mouse&)	override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse&)	override;
				void mouse_enter(graph_reference, const arg_mouse&)	override;
				void mouse_leave(graph_reference, const arg_mouse&)	override;
				void dbl_click(graph_reference, const arg_mouse&)	override;
				void key_press(graph_reference, const arg_keyboard&)override;
				void key_char(graph_reference, const arg_keyboard&)	override;
				void mouse_wheel(graph_reference, const arg_wheel&)	override;
				void resized(graph_reference, const arg_resized&)	override;
				void typeface_changed(graph_reference)				override;
			private:
				void _m_text_area(unsigned width, unsigned height);
			private:
				widget*	widget_;
				widgets::skeletons::text_editor * editor_;
				std::unique_ptr<event_agent>	evt_agent_;
			};
		}//end namespace textbox
	}//end namespace drawerbase

    /// Allow users to enter and edit text by typing on the keyboard.
	class textbox
		:public widget_object<category::widget_tag, drawerbase::textbox::drawer, drawerbase::textbox::textbox_events, ::nana::widgets::skeletons::text_editor_scheme>
	{
	public:
		using colored_area_type = widgets::skeletons::colored_area_type;
		using colored_area_access_interface = widgets::skeletons::colored_area_access_interface;

		using text_focus_behavior = widgets::skeletons::text_focus_behavior;
		using text_positions = std::vector<upoint>;

		using path_type = std::filesystem::path;

		/// The default constructor without creating the widget.
		textbox();

		/// \brief The construct that creates a widget.
		/// @param wd  A handle to the parent window of the widget being created.
		/// @param visible  specifying the visible after creating.
		textbox(window, bool visible);

		/// \brief The construct that creates a widget with a specified text.
		/// @param window  A handle to the parent window of the widget being created.
		/// @param text  the text that will be displayed.
		/// @param visible  specifying the visible after creating.
		textbox(window, const std::string& text, bool visible = true);

		/// \brief The construct that creates a widget with a specified text.
		/// @param window  A handle to the parent window of the widget being created.
		/// @param text  the text that will be displayed.
		/// @param visible  specifying the visible after creating.
		textbox(window, const char* text, bool visible = true);

		/// \brief The construct that creates a widget.
		/// @param window  A handle to the parent window of the widget being created.
		/// @param rectangle  the size and position of the widget in its parent window coordinate.
		/// @param visible  specifying the visible after creating.
		textbox(window, const rectangle& = rectangle(), bool visible = true);

        ///  \brief Loads a text file. When attempt to load a unicode encoded text file, be sure the file have a BOM header.
		void load(const path_type& file);
		void store(const path_type& file);
		void store(const path_type& file, nana::unicode encoding);

		colored_area_access_interface* colored_area_access();

		point content_origin() const;

		/// Enables/disables the textbox to indent a line. Idents a new line when it is created by pressing enter.
		/// @param generator generates text for identing a line. If it is empty, textbox indents the line according to last line.
		textbox& indention(bool, std::function<std::string()> generator = {});

		/// Discards the old text and set a new text. It also clears the filename/edited flags and undo command.
		/// A workaround for reset, explicit default constructor syntax, because VC2013 incorrectly treats {} as {0}.
		/*
		 * @param text A new text replaces the old text.
		 * @param end_caret Indicates whether to position the caret to the end of text.
		 * @return a reference of *this.
		 */
		textbox& reset(const std::string& text = std::string(), bool end_caret = true);      ///< discard the old text and set a new text

		/// The file of last store operation.
		path_type filename() const;

		/// Determine whether the text was edited.
		bool edited() const;

		/// Reset the edited flag to false manually
		textbox& edited_reset();

		/// Determine whether the changed text has been saved into the file.
		bool saved() const;

        /// Read the text from a specified line. It returns true for success.
		bool getline(std::size_t pos, std::string&) const;

		/// Read the text from a specified line with a set offset. It returns true for success.
		bool getline(std::size_t line_index,std::size_t offset,std::string& text) const;

    // Get all text from textbox.
    // It returns a empty string if failed or the textbox is empty.
    std::string text() const { return caption(); }

		/// Read the text from a specified line; returns an empty optional on failure
		std::optional<std::string> getline(std::size_t pos) const;

		///Read the text from a specified line with a set offset. Returns an empty optional for
		/// failure.
		std::optional<std::string> getline(std::size_t line_index, std::size_t offset) const;

		/// Gets the caret position
		/// Returns true if the caret is in the area of display, false otherwise.
		bool caret_pos(point& pos, bool text_coordinate) const;

		/// Gets the caret position, in text coordinate
		upoint caret_pos() const;

		/// Sets the caret position with a text position
		textbox& caret_pos(const upoint&);

        /// Appends an string. If `at_caret` is `true`, the string is inserted at the position of caret, otherwise, it is appended at end of the textbox.
		textbox& append(const std::string& text, bool at_caret);
        textbox& append(const std::wstring& text, bool at_caret);
		/// Determines whether the text is line wrapped.
		bool line_wrapped() const;
		textbox& line_wrapped(bool);

		/// Determines whether the text is multi-line enabled.
		bool multi_lines() const;
		textbox& multi_lines(bool);

		/// Determines whether the textbox accepts user input
		bool editable() const;
		textbox& editable(bool);

		/// Enables the caret if the textbox currently is not editable
		textbox& enable_caret();

		void set_accept(std::function<bool(wchar_t)>);

		textbox& tip_string(::std::string);

        /// Set a mask character. Text is displayed as mask character if a mask character is set. This is used for hiding some special text, such as password.
		textbox& mask(wchar_t);

        /// Returns true if some text is selected.
		bool selected() const;
		bool get_selected_points(nana::upoint &a, nana::upoint &b) const;

        /// Selects/Deselects all text.
		void select(bool);

		void select_points(nana::upoint arg_a, nana::upoint arg_b);

		/// Returns the bounds of a text selection
		/**
		 * @return no selection if pair.first == pair.second.
		 */
		std::pair<upoint, upoint> selection() const;

		void copy() const;  ///< Copies the selected text into shared memory, such as clipboard under Windows.
		void paste();       ///< Pastes the text from shared memory.
		void del();

		int to_int() const;
		double to_double() const;
		textbox& from(int);
		textbox& from(double);

		void clear_undo();

		void set_highlight(const std::string& name, const ::nana::color& fgcolor, const ::nana::color& bgcolor);
		void erase_highlight(const std::string& name);
		void set_keywords(const std::string& name, bool case_sensitive, bool whole_word_match, std::initializer_list<std::wstring> kw_list);
		void set_keywords(const std::string& name, bool case_sensitive, bool whole_word_match, std::initializer_list<std::string> kw_list_utf8);
		void erase_keyword(const std::string& kw);

		/// Sets the text alignment
		textbox& text_align(::nana::align alignment);

		/// Returns the text position of each line that currently displays on screen.
		text_positions text_position() const;

		/// Returns the rectangle of text area
		rectangle text_area() const;

		/// Returns the height of line in pixels
		unsigned line_pixels() const;

		/// Sets the behavior when textbox gets focus.
		void focus_behavior(text_focus_behavior);

		/// Sets the caret move behavior when the content of textbox is selected.
		/// E.g. Whether caret moves to left of selected content or moves to left of last position when left arrow key is pressed.
		/// @param move_to_end determines whether to move caret to left of selected_content or to left of last position.
		void select_behavior(bool move_to_end);

		/// Sets the undo/redo queue length
		/**
		 * @param len The length of the queue. If this parameter is zero, the undo/redo is disabled.
		 */
		void set_undo_queue_length(std::size_t len);

		/// Returns the number of lines that text are displayed in the screen.
		/**
		 * The number of display lines may be not equal to the number of text lines when the textbox
		 * is line wrapped mode.
		 * @return the number of lines that text are displayed in the screen.
		 */
		std::size_t display_line_count() const noexcept;

		/// Returns the number of text lines.
		std::size_t text_line_count() const noexcept;
	protected:
		//Overrides widget's virtual functions
		native_string_type _m_caption() const noexcept override;
		void _m_caption(native_string_type&&) override;
		void _m_typeface(const paint::font&) override;
		std::shared_ptr<scroll_operation_interface> _m_scroll_operation() override;
	};
}//end namespace nana
#include <nana/pop_ignore_diagnostic>

#endif
