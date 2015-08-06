/*
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

#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/skeletons/text_editor.hpp>
#include <stdexcept>
#include <sstream>

#include <nana/gui/detail/bedrock.hpp>
#include <nana/gui/detail/inner_fwd_implement.hpp>

namespace nana
{
	arg_textbox::arg_textbox(textbox& wdg)
		: widget(wdg)
	{}

namespace drawerbase {
	namespace textbox
	{
		//class event_agent
			event_agent::event_agent(::nana::textbox& wdg)
				:widget_(wdg)
			{}

			void event_agent::first_change()
			{
				widget_.events().first_change.emit(::nana::arg_textbox{ widget_ });
			}

			void event_agent::text_changed()
			{
				widget_.events().text_changed.emit(::nana::arg_textbox{ widget_ });
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
			evt_agent_.reset(new event_agent(static_cast< ::nana::textbox&>(wdg)));

			auto scheme = API::dev::get_scheme(wdg);

			editor_ = new text_editor(wd, graph, dynamic_cast<::nana::widgets::skeletons::text_editor_scheme*>(scheme));
			editor_->textbase().set_event_agent(evt_agent_.get());

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
			if(editor_->mouse_down(arg.button, arg.pos))
				API::lazy_refresh();
		}

		void drawer::mouse_move(graph_reference, const arg_mouse& arg)
		{
			if(editor_->mouse_move(arg.left_button, arg.pos))
				API::lazy_refresh();
		}

		void drawer::mouse_up(graph_reference graph, const arg_mouse& arg)
		{
			if(editor_->mouse_up(arg.button, arg.pos))
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

		textbox::textbox(window wd, const nana::string& text, bool visible)
		{
			create(wd, rectangle(), visible);
			caption(text);
		}

		textbox::textbox(window wd, const nana::char_t* text, bool visible)
		{
			create(wd, rectangle(), visible);
			caption(text);
		}

		textbox::textbox(window wd, const rectangle& r, bool visible)
		{
			create(wd, r, visible);
		}

		void textbox::load(nana::string file)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor && editor->load(file.data()))
				API::update_window(handle());
		}

		void textbox::store(nana::string file)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->textbase().store(std::move(file));
		}

		void textbox::store(nana::string file, nana::unicode encoding)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->textbase().store(std::move(file), encoding);
		}

		textbox& textbox::reset(nana::string str)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
			{
				editor->text(std::move(str));
				editor->textbase().reset();
				API::update_window(this->handle());
			}
			return *this;
		}

		nana::string textbox::filename() const
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

		bool textbox::getline(std::size_t line_index, nana::string& text) const
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			return (editor ? editor->getline(line_index, text) : false);
		}

		/// Gets the caret position
		bool textbox::caret_pos(point& pos, bool text_coordinate) const
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();

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

		textbox& textbox::append(const nana::string& text, bool at_caret)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if(editor)
			{
				if(at_caret == false)
					editor->move_caret_end();

				editor->put(text);
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

		void textbox::set_accept(std::function<bool(nana::char_t)> fn)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if(editor)
				editor->set_accept(std::move(fn));
		}

		textbox& textbox::tip_string(nana::string str)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if(editor && editor->tip_string(std::move(str)))
				API::refresh_window(handle());
			return *this;
		}

		textbox& textbox::mask(nana::char_t ch)
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
			nana::string s = _m_caption();
			if (s.empty()) return 0;

#ifdef NANA_UNICODE
			std::wstringstream ss;
#else
			std::stringstream ss;
#endif
			int value;
			ss << s;
			ss >> value;
			return value;
		}

		double textbox::to_double() const
		{
			nana::string s = _m_caption();
			if (s.empty()) return 0;

#ifdef NANA_UNICODE
			std::wstringstream ss;
#else
			std::stringstream ss;
#endif
			double value;
			ss << s;
			ss >> value;
			return value;
		}

		textbox& textbox::from(int n)
		{
			_m_caption(std::to_wstring(n));
			return *this;
		}

		textbox& textbox::from(double d)
		{
			_m_caption(std::to_wstring(d));
			return *this;
		}

		void textbox::set_highlight(const std::string& name, const ::nana::color& fgcolor, const ::nana::color& bgcolor)
		{
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->set_highlight(name, fgcolor, bgcolor);
		}

		void textbox::erase_highlight(const std::string& name)
		{
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->erase_highlight(name);
		}

		void textbox::set_keywords(const std::string& name, bool case_sensitive, bool whole_word_match, std::initializer_list<nana::string> kw_list)
		{
			auto editor = get_drawer_trigger().editor();
			if (editor)
			{
				for (auto & kw : kw_list)
					editor->set_keyword(kw, name, case_sensitive, whole_word_match);
			}
		}

		void textbox::set_keywords(const std::string& name, bool case_sensitive, bool whole_word_match, std::initializer_list<std::string> kw_list_utf8)
		{
			auto editor = get_drawer_trigger().editor();
			if (editor)
			{
				for (auto & kw : kw_list_utf8)
					editor->set_keyword(::nana::charset(kw, ::nana::unicode::utf8), name, case_sensitive, whole_word_match);
			}
		}

		void textbox::erase_keyword(const nana::string& kw)
		{
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->erase_keyword(kw);
		}

		//Override _m_caption for caption()
		nana::string textbox::_m_caption() const throw()
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			return (editor ? editor->text() : nana::string());
		}

		void textbox::_m_caption(nana::string&& str)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
			{
				editor->text(std::move(str));
				API::update_window(this->handle());
			}
		}

		//Override _m_typeface for changing the caret
		void textbox::_m_typeface(const nana::paint::font& font)
		{
			widget::_m_typeface(font);
			auto editor = get_drawer_trigger().editor();
			if(editor)
				editor->reset_caret_height();
		}
	//end class textbox
}//end namespace nana

