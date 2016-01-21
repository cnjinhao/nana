/*
 *	A Textbox Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/textbox.hpp
 */

#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/skeletons/text_editor.hpp>
#include <stdexcept>
#include <sstream>

namespace nana
{
	arg_textbox::arg_textbox(textbox& wdg, const std::vector<upoint>& text_pos)
		: widget(wdg), text_position(text_pos)
	{}

namespace drawerbase {
	namespace textbox
	{
		//class event_agent
			event_agent::event_agent(::nana::textbox& wdg, const std::vector<upoint>& text_pos)
				:widget_(wdg), text_position_(text_pos)
			{}

			void event_agent::first_change()
			{
				widget_.events().first_change.emit(::nana::arg_textbox{ widget_, text_position_ });
			}

			void event_agent::text_changed()
			{
				widget_.events().text_changed.emit(::nana::arg_textbox{ widget_, text_position_ });
			}

			void event_agent::text_exposed(const std::vector<upoint>& text_pos)
			{
				::nana::arg_textbox arg(widget_, text_pos);
				widget_.events().text_exposed.emit(arg);
			}
		//end class event_agent

	//class draweer
		drawer::drawer()
			: widget_(nullptr), editor_(nullptr)
		{
		}

		drawer::text_editor* drawer::editor()
		{
			return editor_;
		}

		const drawer::text_editor* drawer::editor() const
		{
			return editor_;
		}

		void drawer::attached(widget_reference wdg, graph_reference graph)
		{
			auto wd = wdg.handle();
			widget_ = &wdg;
			evt_agent_.reset(new event_agent(static_cast<::nana::textbox&>(wdg), editor_->text_position()));

			auto scheme = API::dev::get_scheme(wdg);

			editor_ = new text_editor(wd, graph, dynamic_cast<::nana::widgets::skeletons::text_editor_scheme*>(scheme));
			editor_->textbase().set_event_agent(evt_agent_.get());
			editor_->set_event(evt_agent_.get());

			_m_text_area(graph.width(), graph.height());

			API::tabstop(wd);
			API::eat_tabstop(wd, true);
			API::effects_edge_nimbus(wd, effects::edge_nimbus::active);
			API::effects_edge_nimbus(wd, effects::edge_nimbus::over);
		}

		void drawer::detached()
		{
			delete editor_;
			editor_ = nullptr;
		}

		void drawer::refresh(graph_reference graph)
		{
			editor_->render(API::is_focus_ready(*widget_));
		}

		void drawer::focus(graph_reference graph, const arg_focus& arg)
		{
			refresh(graph);
			if (!editor_->attr().multi_lines && arg.getting)
			{
				editor_->select(true);
				editor_->move_caret_end();
			}
			editor_->show_caret(arg.getting);
			editor_->reset_caret();
			API::lazy_refresh();
		}

		void drawer::mouse_down(graph_reference, const arg_mouse& arg)
		{
			if (editor_->mouse_pressed(arg))
			{
				editor_->render(true);
				API::lazy_refresh();
			}
		}

		void drawer::mouse_move(graph_reference, const arg_mouse& arg)
		{
			if(editor_->mouse_move(arg.left_button, arg.pos))
				API::lazy_refresh();
		}

		void drawer::mouse_up(graph_reference graph, const arg_mouse& arg)
		{
			if(editor_->mouse_pressed(arg))
				API::lazy_refresh();
		}

		void drawer::mouse_wheel(graph_reference, const arg_wheel& arg)
		{
			if(editor_->scroll(arg.upwards, true))
			{
				editor_->reset_caret();
				API::lazy_refresh();
			}
		}

		void drawer::mouse_enter(graph_reference, const arg_mouse&)
		{
			if(editor_->mouse_enter(true))
				API::lazy_refresh();
		}

		void drawer::mouse_leave(graph_reference, const arg_mouse&)
		{
			if(editor_->mouse_enter(false))
				API::lazy_refresh();
		}

		void drawer::key_press(graph_reference, const arg_keyboard& arg)
		{
			if(editor_->respond_key(arg))
			{
				editor_->reset_caret();
				API::lazy_refresh();
			}
		}

		void drawer::key_char(graph_reference, const arg_keyboard& arg)
		{
			if (editor_->respond_char(arg))
				API::lazy_refresh();
		}

		void drawer::resized(graph_reference graph, const arg_resized& arg)
		{
			_m_text_area(arg.width, arg.height);
			refresh(graph);
			editor_->reset_caret();
			API::lazy_refresh();
		}

		void drawer::typeface_changed(graph_reference graph)
		{
			editor_->typeface_changed();
			refresh(graph);
			API::update_window(widget_->handle());
		}

		void drawer::_m_text_area(unsigned width, unsigned height)
		{
			if(editor_)
			{
				nana::rectangle r(0, 0, width, height);

				if (!API::widget_borderless(widget_->handle()))
				{
					r.x = r.y = 2;
					r.width = (width > 4 ? width - 4 : 0);
					r.height = (height > 4 ? height - 4 : 0);
				}
				editor_->text_area(r);
			}
		}
	//end class drawer
}//end namespace textbox
}//end namespace drawerbase

	//class textbox
		textbox::textbox()
		{}

		textbox::textbox(window wd, bool visible)
		{
			create(wd, rectangle(), visible);
		}

		textbox::textbox(window wd, const std::string& text, bool visible)
		{
			throw_not_utf8(text);
			create(wd, rectangle(), visible);
			caption(text);
		}

		textbox::textbox(window wd, const char* text, bool visible)
		{
			throw_not_utf8(text);
			create(wd, rectangle(), visible);
			caption(text);
		}

		textbox::textbox(window wd, const rectangle& r, bool visible)
		{
			create(wd, r, visible);
		}

		void textbox::load(std::string file)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor && editor->load(file.data()))
				API::update_window(handle());
		}

		void textbox::store(std::string file)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->textbase().store(std::move(file), false, nana::unicode::utf8);	//3rd parameter is just for syntax, it will be ignored
		}

		void textbox::store(std::string file, nana::unicode encoding)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->textbase().store(std::move(file), true, encoding);
		}

		/// Enables/disables the textbox to indent a line. Idents a new line when it is created by pressing enter.
		/// @param generator generates text for identing a line. If it is empty, textbox indents the line according to last line.
		textbox& textbox::indention(bool enb, std::function<std::string()> generator)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->indent(enb, generator);
			return *this;
		}

		textbox& textbox::reset(const std::string& str)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
			{
				editor->text(to_wstring(str));
				editor->textbase().reset();
				API::update_window(this->handle());
			}
			return *this;
		}

		std::string textbox::filename() const
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if(editor)
				return editor->textbase().filename();

			return{};
		}

		bool textbox::edited() const
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			return (editor ? editor->textbase().edited() : false);
		}

		textbox& textbox::edited_reset()
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->textbase().edited_reset();

			return *this;
		}

		bool textbox::saved() const
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			return (editor ? editor->textbase().saved() : false);
		}

		bool textbox::getline(std::size_t line_index, std::string& text) const
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
			{
				std::wstring line_text;
				if (editor->getline(line_index, line_text))
				{
					text = to_utf8(line_text);
					return true;
				}
			}
			return false;
		}

		/// Gets the caret position
		bool textbox::caret_pos(point& pos, bool text_coordinate) const
		{
			auto editor = get_drawer_trigger().editor();
			internal_scope_guard lock;
			if (!editor)
				return false;

			auto scr_pos = editor->caret_screen_pos();

			if (text_coordinate)
			{
				auto upos = editor->caret();
				pos.x = static_cast<int>(upos.x);
				pos.y = static_cast<int>(upos.y);
			}
			else
				pos = scr_pos;

			return editor->hit_text_area(scr_pos);
		}

		textbox& textbox::caret_pos(const upoint& pos)
		{
			auto editor = get_drawer_trigger().editor();
			internal_scope_guard lock;
			if (editor)
				editor->move_caret(pos);
			
			return *this;
		}

		textbox& textbox::append(const std::string& text, bool at_caret)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if(editor)
			{
				if(at_caret == false)
					editor->move_caret_end();

				editor->put(to_wstring(text));
				API::update_window(this->handle());
			}
			return *this;
		}

		/// Determine wheter the text is auto-line changed.
		bool textbox::line_wrapped() const
		{
			internal_scope_guard lock;
			return get_drawer_trigger().editor()->line_wrapped();
		}

		textbox& textbox::line_wrapped(bool autl)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor->line_wrapped(autl))
				API::update_window(handle());

			return *this;
		}

		bool textbox::multi_lines() const
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			return (editor ? editor->attr().multi_lines : false);
		}

		textbox& textbox::multi_lines(bool ml)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if(editor && editor->multi_lines(ml))
				API::update_window(handle());
			return *this;
		}

		bool textbox::editable() const
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			return (editor ? editor->attr().editable : false);
		}

		textbox& textbox::editable(bool able)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if(editor)
				editor->editable(able);
			return *this;
		}

		void textbox::set_accept(std::function<bool(wchar_t)> fn)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if(editor)
				editor->set_accept(std::move(fn));
		}

		textbox& textbox::tip_string(std::string str)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if(editor && editor->tip_string(std::move(str)))
				API::refresh_window(handle());
			return *this;
		}

		textbox& textbox::mask(wchar_t ch)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if(editor && editor->mask(ch))
				API::refresh_window(handle());
			return *this;
		}

		bool textbox::selected() const
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			return (editor ? editor->selected() : false);
		}

		void textbox::select(bool yes)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if(editor && editor->select(yes))
				API::update_window(*this);
		}

		void textbox::copy() const
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if(editor)
				editor->copy();
		}

		void textbox::paste()
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if(editor)
			{
				editor->paste();
				API::update_window(*this);
			}
		}

		void textbox::del()
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if(editor)
			{
				editor->del();
				API::refresh_window(*this);
			}
		}

		int textbox::to_int() const
		{
			auto s = _m_caption();
			if (s.empty()) return 0;

			std::stringstream ss;
			int value;
			ss << to_utf8(s);
			ss >> value;
			return value;
		}

		double textbox::to_double() const
		{
			auto s = _m_caption();
			if (s.empty()) return 0;

			std::stringstream ss;
			double value;
			ss << to_utf8(s);
			ss >> value;
			return value;
		}

		textbox& textbox::from(int n)
		{
			_m_caption(to_nstring(n));
			return *this;
		}

		textbox& textbox::from(double d)
		{
			_m_caption(to_nstring(d));
			return *this;
		}

		void textbox::set_highlight(const std::string& name, const ::nana::color& fgcolor, const ::nana::color& bgcolor)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->set_highlight(name, fgcolor, bgcolor);
		}

		void textbox::erase_highlight(const std::string& name)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->erase_highlight(name);
		}

		void textbox::set_keywords(const std::string& name, bool case_sensitive, bool whole_word_match, std::initializer_list<std::wstring> kw_list)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
			{
				for (auto & kw : kw_list)
					editor->set_keyword(kw, name, case_sensitive, whole_word_match);
			}
		}

		void textbox::set_keywords(const std::string& name, bool case_sensitive, bool whole_word_match, std::initializer_list<std::string> kw_list_utf8)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
			{
				for (auto & kw : kw_list_utf8)
					editor->set_keyword(::nana::charset(kw, ::nana::unicode::utf8), name, case_sensitive, whole_word_match);
			}
		}

		void textbox::erase_keyword(const std::string& kw)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->erase_keyword(to_wstring(kw));
		}

		std::vector<upoint> textbox::text_position() const
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				return editor->text_position();
			
			return{};
		}

		rectangle textbox::text_area() const
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				return editor->text_area(false);

			return{};
		}

		unsigned textbox::line_pixels() const
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			return (editor ? editor->line_height() : 0);
		}

		//Override _m_caption for caption()
		auto textbox::_m_caption() const throw() -> native_string_type
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				return to_nstring(editor->text());

			return native_string_type();
		}

		void textbox::_m_caption(native_string_type&& str)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
			{
				editor->text(to_wstring(str));
				API::update_window(this->handle());
			}
		}

		//Override _m_typeface for changing the caret
		void textbox::_m_typeface(const nana::paint::font& font)
		{
			widget::_m_typeface(font);
			auto editor = get_drawer_trigger().editor();
			if(editor)
				editor->reset_caret_pixels();
		}
	//end class textbox
}//end namespace nana

