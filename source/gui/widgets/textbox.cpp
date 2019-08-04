/*
 *	A Textbox Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/textbox.hpp
 *	@contributors: Oleg Smolsky
 */

#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/skeletons/text_editor.hpp>
#include <stdexcept>

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
				widget_.events().first_change.emit(::nana::arg_textbox{ widget_, text_position_ }, widget_);
			}

			void event_agent::text_changed()
			{
				widget_.events().text_changed.emit(::nana::arg_textbox{ widget_, text_position_ }, widget_);
			}

			void event_agent::text_exposed(const std::vector<upoint>& text_pos)
			{
				::nana::arg_textbox arg(widget_, text_pos);
				widget_.events().text_exposed.emit(arg, widget_);
			}
		//end class event_agent

	//class drawer
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

			auto scheme = API::dev::get_scheme(wdg);

			editor_ = new text_editor(wd, graph, dynamic_cast<::nana::widgets::skeletons::text_editor_scheme*>(scheme));

			evt_agent_.reset(new event_agent(static_cast<::nana::textbox&>(wdg), editor_->text_position()));
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

		void drawer::refresh(graph_reference)
		{
			editor_->render(API::is_focus_ready(*widget_));
		}

		void drawer::focus(graph_reference graph, const arg_focus& arg)
		{
			if (!editor_->focus_changed(arg))
				refresh(graph);

			API::dev::lazy_refresh();
		}

		void drawer::mouse_down(graph_reference, const arg_mouse& arg)
		{
			editor_->mouse_pressed(arg);
			if(editor_->try_refresh())
				API::dev::lazy_refresh();
		}

		void drawer::mouse_move(graph_reference, const arg_mouse& arg)
		{
			editor_->mouse_move(arg.left_button, arg.pos);
			if(editor_->try_refresh())
				API::dev::lazy_refresh();
		}

		void drawer::mouse_up(graph_reference, const arg_mouse& arg)
		{
			editor_->mouse_pressed(arg);
			if(editor_->try_refresh())
				API::dev::lazy_refresh();
		}

		void drawer::mouse_wheel(graph_reference, const arg_wheel& arg)
		{
			if(editor_->scroll(arg.upwards, true))
			{
				editor_->reset_caret();
				API::dev::lazy_refresh();
			}
		}

		void drawer::mouse_enter(graph_reference, const arg_mouse&)
		{
			if(editor_->mouse_enter(true))
				API::dev::lazy_refresh();
		}

		void drawer::mouse_leave(graph_reference, const arg_mouse&)
		{
			if(editor_->mouse_enter(false))
				API::dev::lazy_refresh();
		}

		//Added Windows-style mouse double-click to the textbox(https://github.com/cnjinhao/nana/pull/229)
		//Oleg Smolsky
		void drawer::dbl_click(graph_reference, const arg_mouse& arg)
		{
			if(editor_->select_word(arg))
				API::dev::lazy_refresh();
		}

		void drawer::key_ime(graph_reference, const arg_ime& arg)
		{
			editor_->respond_ime(arg);
			if (editor_->try_refresh())
				API::dev::lazy_refresh();
		}

		void drawer::key_press(graph_reference, const arg_keyboard& arg)
		{
			editor_->respond_key(arg);
			editor_->reset_caret(true);
			if(editor_->try_refresh())
				API::dev::lazy_refresh();
		}

		void drawer::key_char(graph_reference, const arg_keyboard& arg)
		{
			editor_->respond_char(arg);
			if(editor_->try_refresh())
				API::dev::lazy_refresh();
		}

		void drawer::resized(graph_reference graph, const arg_resized& arg)
		{
			_m_text_area(arg.width, arg.height);
			refresh(graph);
			editor_->reset_caret();

			if (!editor_->try_refresh())
				refresh(graph);

			API::dev::lazy_refresh();
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

		void textbox::load(const std::filesystem::path& file)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor && editor->load(file))
			{
				if (editor->try_refresh())
					API::update_window(handle());
			}
		}

		void textbox::store(const std::filesystem::path& file)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->textbase().store(file, false, nana::unicode::utf8);	//3rd parameter is just for syntax, it will be ignored
		}

		void textbox::store(const std::filesystem::path& file, nana::unicode encoding)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->textbase().store(file, true, encoding);
		}

		textbox::colored_area_access_interface* textbox::colored_area_access()
		{
			auto editor = get_drawer_trigger().editor();
			if (editor)
				return &editor->colored_area();

			return nullptr;
		}

		point textbox::content_origin() const
		{
			auto editor = get_drawer_trigger().editor();
			if (editor)
				return editor->content_origin();

			return{};
		}

		/// Enables/disables the textbox to indent a line. Indents a new line when it is created by pressing enter.
		/// @param generator generates text for indenting a line. If it is empty, textbox indents the line according to last line.
		textbox& textbox::indention(bool enb, std::function<std::string()> generator)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->indent(enb, generator);
			return *this;
		}

		textbox& textbox::reset(const std::string& str, bool end_caret)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
			{
				editor->text(to_wstring(str), false);
				
				if (end_caret)
					editor->move_caret_end(true);

				//Reset the edited status and the saved filename
				editor->textbase().reset_status(false);

				if (editor->try_refresh())
					API::update_window(this->handle());
			}
			return *this;
		}

		textbox::path_type textbox::filename() const
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
				editor->textbase().reset_status(true);

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

		bool textbox::getline(std::size_t line_index,std::size_t start_point,std::string& text) const
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if(editor)
			{
				std::wstring line_text;
				if(editor->getline(line_index,line_text))
				{
					if(line_text.length() >= start_point)
					{
						text = to_utf8(line_text.substr(start_point));
						return true;
					}
				}
			}
			return false;
		}

		std::optional<std::string> textbox::getline(std::size_t pos) const
		{
			auto result = std::string{};
			if ( getline(pos, result) )
			{
				return { std::move(result) };
			}
			return {};
		}

		std::optional<std::string> textbox::getline(std::size_t line_index, std::size_t offset) const
		{
			auto result = std::string{};
			if ( getline(line_index, offset, result) )
			{
				return { std::move(result) };
			}
			return {};
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

		upoint textbox::caret_pos() const
		{
			auto editor = get_drawer_trigger().editor();
			internal_scope_guard lock;
			if (editor)
				return editor->caret();

			return{};
		}

		textbox& textbox::caret_pos(const upoint& pos)
		{
			auto editor = get_drawer_trigger().editor();
			internal_scope_guard lock;
			if (editor && editor->move_caret(pos, true))
				API::refresh_window(handle());
			
			return *this;
		}

		textbox& textbox::append(const std::string& text, bool at_caret)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if(editor)
			{
				if(at_caret == false)
					editor->move_caret_end(false);

				editor->put(to_wstring(text), true);

				editor->try_refresh();
				API::update_window(this->handle());
			}
			return *this;
		}
        //a native wstring version textbox::append
        textbox& textbox::append(const std::wstring& text, bool at_caret)
        {
            internal_scope_guard lock;
            auto editor = get_drawer_trigger().editor();
            if(editor)
            {
                if(at_caret == false)
                    editor->move_caret_end(false);

                editor->put(text, true);

                editor->try_refresh();
                API::update_window(this->handle());
            }
            return *this;
        }
		/// Determine whether the text is auto-line changed.
		bool textbox::line_wrapped() const
		{
			internal_scope_guard lock;
			return get_drawer_trigger().editor()->attr().line_wrapped;
		}

		textbox& textbox::line_wrapped(bool autl)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor && editor->line_wrapped(autl))
			{
				editor->try_refresh();
				API::update_window(handle());
			}

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
			if (editor && editor->multi_lines(ml))
			{
				auto wd = handle();
				API::eat_tabstop(wd, ml);	//textbox handles the Tab pressing when it is multi-line.

				editor->try_refresh();
				API::update_window(wd);
			}
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
				editor->editable(able, false);
			return *this;
		}

		textbox& textbox::enable_caret()
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->editable(editor->attr().editable, true);
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

		bool textbox::get_selected_points(nana::upoint &a, nana::upoint &b) const
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			return (editor ? editor->get_selected_points(a, b) : false);
		}

		void textbox::select(bool yes)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor && editor->select(yes))
				API::refresh_window(*this);
		}


		void textbox::select_points(nana::upoint arg_a, nana::upoint arg_b)
		{
			auto editor = get_drawer_trigger().editor();
			internal_scope_guard lock;
			if (editor && editor->select_points(arg_a, arg_b))
				API::refresh_window(*this);
		}

		std::pair<upoint, upoint> textbox::selection() const
		{
			std::pair<upoint, upoint> points;

			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->get_selected_points(points.first, points.second);

			return points;
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
				if (editor->try_refresh())
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

			return std::stoi(s, nullptr, 0);
		}

		double textbox::to_double() const
		{
			auto s = _m_caption();
			if (s.empty()) return 0;

			return std::stod(s);
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

		void textbox::clear_undo()
		{
			get_drawer_trigger().editor()->undo_clear();
		}

		void textbox::set_highlight(const std::string& name, const ::nana::color& fgcolor, const ::nana::color& bgcolor)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
			{
				editor->set_highlight(name, fgcolor, bgcolor);
				API::refresh_window(handle());
			}
		}

		void textbox::erase_highlight(const std::string& name)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
			{
				editor->erase_highlight(name);
				API::refresh_window(handle());
			}
		}

		void textbox::set_keywords(const std::string& name, bool case_sensitive, bool whole_word_match, std::initializer_list<std::wstring> kw_list)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
			{
				for (auto & kw : kw_list)
					editor->set_keyword(kw, name, case_sensitive, whole_word_match);
				API::refresh_window(handle());
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
				API::refresh_window(handle());
			}
		}

		void textbox::erase_keyword(const std::string& kw)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
			{
				editor->erase_keyword(to_wstring(kw));
				API::refresh_window(handle());
			}
		}

		textbox& textbox::text_align(::nana::align alignment)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
			{
				editor->text_align(alignment);
				API::refresh_window(handle());
			}

			return *this;
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

		void textbox::focus_behavior(text_focus_behavior behavior)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->focus_behavior(behavior);
		}

		void textbox::select_behavior(bool move_to_end)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->select_behavior(move_to_end);
		}

		void textbox::set_undo_queue_length(std::size_t len)
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				editor->undo_max_steps(len);
		}

		std::size_t textbox::display_line_count() const noexcept
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				return editor->line_count(false);

			return 0;
		}

		std::size_t textbox::text_line_count() const noexcept
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				return editor->line_count(true);

			return 0;
		}

		//Override _m_caption for caption()
		auto textbox::_m_caption() const noexcept -> native_string_type
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
				editor->text(to_wstring(str), false);

				if (editor->try_refresh())
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

		std::shared_ptr<scroll_operation_interface> textbox::_m_scroll_operation()
		{
			internal_scope_guard lock;
			auto editor = get_drawer_trigger().editor();
			if (editor)
				return editor->scroll_operation();

			return {};
		}
	//end class textbox
}//end namespace nana

