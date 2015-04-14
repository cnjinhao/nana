/**
 *	A Textbox Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/textbox.hpp
 */
#ifndef NANA_GUI_WIDGET_TEXTBOX_HPP
#define NANA_GUI_WIDGET_TEXTBOX_HPP
#include <nana/gui/widgets/widget.hpp>
#include "skeletons/textbase_export_interface.hpp"
#include "skeletons/text_editor_scheme.hpp"

namespace nana
{
	class textbox;

	struct arg_textbox
		: public event_arg
	{
		textbox& widget;

		arg_textbox(textbox&);
	};

	namespace widgets
	{
		namespace skeletons
		{
			class text_editor;
		}
	}

	namespace drawerbase
	{
		namespace textbox
		{
			struct textbox_events
				: public general_events
			{
				basic_event<arg_textbox> first_change;
				basic_event<arg_textbox> text_changed;
			};

			class event_agent
				: public widgets::skeletons::textbase_event_agent_interface
			{
			public:
				event_agent(::nana::textbox&);
				void first_change() override;
				void text_changed() override;
			private:
				::nana::textbox & widget_;
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
		textbox(window, const nana::string& text, bool visible = true);

		/// \brief The construct that creates a widget with a specified text.
		/// @param window  A handle to the parent window of the widget being created.
		/// @param text  the text that will be displayed.
		/// @param visible  specifying the visible after creating.
		textbox(window, const nana::char_t* text, bool visible = true);

		/// \brief The construct that creates a widget.
		/// @param window  A handle to the parent window of the widget being created.
		/// @param rectangle  the size and position of the widget in its parent window coordinate.
		/// @param visible  specifying the visible after creating.
		textbox(window, const rectangle& = rectangle(), bool visible = true);

        ///  \brief Loads a text file. When attempt to load a unicode encoded text file, be sure the file have a BOM header.
		void load(nana::string file);
		void store(nana::string file);
		void store(nana::string file, nana::unicode encoding);

		//A workaround for reset, explicit default constructor syntax, because VC2013 incorrectly treats {} as {0}.
		textbox& reset(nana::string = nana::string());      ///< discard the old text and set a newtext

		/// The file of last store operation.
		nana::string filename() const;

		/// Determine whether the text was edited.
		bool edited() const;

		/// Reset the edited flag to false manually
		textbox& edited_reset();

		/// Determine whether the changed text has been saved into the file.
		bool saved() const;

        /// Read the text from a specified line. It returns true for success.
		bool getline(std::size_t line_index, nana::string&) const;

        /// Appends an string. If `at_caret` is `true`, the string is inserted at the position of caret, otherwise, it is appended at end of the textbox.
		textbox& append(const nana::string& text, bool at_caret);

		/// Determine wheter the text is line wrapped. 
		bool line_wrapped() const;
		textbox& line_wrapped(bool);

		/// Determine whether the text is multi-line enabled.
		bool multi_lines() const;
		textbox& multi_lines(bool);
		bool editable() const;
		textbox& editable(bool);
		void set_accept(std::function<bool(nana::char_t)>);

		textbox& tip_string(nana::string);

        /// Set a mask character. Text is displayed as mask character if a mask character is set. This is used for hiding some special text, such as password.
		textbox& mask(nana::char_t);

        /// Returns true if some text is selected.
		bool selected() const;

        /// Selects/unselects all text.
		void select(bool);

		void copy() const;  ///< Copies the selected text into shared memory, such as clipboard under Windows.
		void paste();       ///< Pastes the text from shared memory.
		void del();

		int to_int() const;
		double to_double() const;
		textbox& from(int);
		textbox& from(double);

		void set_highlight(const std::string& name, const ::nana::color& fgcolor, const ::nana::color& bgcolor);
		void erase_highlight(const std::string& name);
		void set_keywords(const std::string& name, bool case_sensitive, bool whole_word_match, std::initializer_list<nana::string> kw_list);
		void set_keywords(const std::string& name, bool case_sensitive, bool whole_word_match, std::initializer_list<std::string> kw_list_utf8);
		void erase_keyword(const nana::string& kw);
	protected:
		//Overrides widget's virtual functions
		::nana::string _m_caption() const override;
		void _m_caption(::nana::string&&) override;
		void _m_typeface(const paint::font&) override;
	};

	namespace dev
	{
		/// Traits for widget classes
		template<>
		struct widget_traits<textbox>
		{
			using event_type = drawerbase::textbox::textbox_events;
			using scheme_type = ::nana::widgets::skeletons::text_editor_scheme;
		};
	}
}//end namespace nana
#endif
