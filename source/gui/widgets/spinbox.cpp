/*
 *	A Spin box widget
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/spinbox.cpp
 */

#include <nana/gui/widgets/spinbox.hpp>
#include <nana/gui/widgets/skeletons/text_editor.hpp>
#include <nana/gui/element.hpp>
#include <nana/gui/timer.hpp>
#include <algorithm>

namespace nana
{
	arg_spinbox::arg_spinbox(spinbox& wdg): widget(wdg)
	{}

	namespace drawerbase
	{
		namespace spinbox
		{

			class event_agent
				: public widgets::skeletons::textbase_event_agent_interface
			{
			public:
				event_agent(::nana::spinbox& wdg)
					: widget_(wdg)
				{}

				void first_change() override{}	//empty, because spinbox does not have this event.

				void text_changed() override
				{
					widget_.events().text_changed.emit(::nana::arg_spinbox{ widget_ });
				}
			private:
				::nana::spinbox & widget_;
			};

			enum class buttons
			{
				none, increase, decrease
			};

			class range_interface
			{
			public:
				virtual ~range_interface() = default;

				virtual std::string value() const = 0;

				//sets a new value, the diff indicates whether the new value is different from the current value.
				//returns true if the new value is acceptable.
				virtual bool value(const std::string& new_value, bool& diff) = 0;

				virtual bool check_value(const std::string&) const = 0;
				virtual void spin(bool increase) = 0;
			};

			template<typename T>
			class range_numeric
				: public range_interface
			{
			public:
				range_numeric(T vbegin, T vlast, T step)
					: begin_{ vbegin }, last_{ vlast }, step_{ step }, value_{ vbegin }
				{}

				std::string value() const override
				{
					return std::to_string(value_);
				}

				bool value(const std::string& value_str, bool & diff) override
				{
					std::stringstream ss;
					ss << value_str;

					T v;
					ss >> v;
					if (v < begin_ || last_ < v)
						return false;

					diff = (value_ != v);
					value_ = v;
					return true;
				}

				bool check_value(const std::string& str) const override
				{
					if (str.empty())
						return true;

					auto size = str.size();
					std::size_t pos = 0;
					if (str[0] == '+' || str[0] == '-')
						pos = 1;

					if (std::is_same<T, int>::value)
					{
						for (; pos < size; ++pos)
						{
							auto ch = str[pos];
							if (ch < '0' || '9' < ch)
								return false;
						}
					}
					else
					{
						bool dot = false;
						for (; pos < size; ++pos)
						{
							auto ch = str[pos];
							if (('.' == ch) && (!dot))
							{
								dot = true;
								continue;
							}

							if (ch < '0' || '9' < ch)
								return false;
						}
					}
					return true;
				}

				void spin(bool increase) override
				{
					if (increase)
					{
						value_ += step_;
						if (value_ > last_)
							value_ = last_;
					}
					else
					{
						value_ -= step_;
						if (value_ < begin_)
							value_ = begin_;
					}
				}
			private:
				T begin_;
				T last_;
				T step_;
				T value_;
			};

			class range_text
				: public range_interface
			{
			public:
				range_text(std::initializer_list<std::string> & initlist)
					: texts_(initlist)
				{
					for (auto & s : initlist)
					{
						texts_.emplace_back(::nana::charset(s, ::nana::unicode::utf8));
					}
				}

				range_text(std::initializer_list<std::wstring>& initlist)
				{
					for (auto & s : initlist)
						texts_.emplace_back(to_utf8(s));
				}

				std::string value() const override
				{
					if (texts_.empty())
						return{};

					return texts_[pos_];
				}

				bool value(const std::string& value_str, bool & diff) override
				{
					auto i = std::find(texts_.cbegin(), texts_.cend(), value_str);
					if (i != texts_.cend())
					{
						diff = (*i == value_str);
						pos_ = i - texts_.cbegin();
						return true;
					}
					return false;
				}

				bool check_value(const std::string& str) const override
				{
					if (str.empty())
						return true;

					for (auto i = texts_.cbegin(); i != texts_.cend(); ++i)
						if (i->find(str) != str.npos)
							return false;

					return true;
				}

				void spin(bool increase) override
				{
					if (texts_.empty())
						return;

					if (increase)
					{
						++pos_;
						if (texts_.size() <= pos_)
							pos_ = texts_.size() - 1;
					}
					else
					{
						--pos_;
						if (texts_.size() <= pos_)
							pos_ = 0;
					}
				}
			private:
				std::vector<std::string> texts_;
				std::size_t pos_{0};
			};

			class implementation
			{
			public:
				implementation()
				{
					//Sets a timer for continous spin when mouse button is pressed.
					timer_.elapse([this]
					{
						range_->spin(buttons::increase == spin_stated_);
						reset_text();
						API::update_window(editor_->window_handle());

						auto intv = timer_.interval();
						if (intv > 50)
							timer_.interval(intv / 2);
					});

					timer_.interval(600);
				}

				void attach(::nana::widget& wdg, ::nana::paint::graphics& graph)
				{
					auto wd = wdg.handle();
					graph_ = &graph;
					auto scheme = static_cast<::nana::widgets::skeletons::text_editor_scheme*>(API::dev::get_scheme(wd));
					editor_ = new ::nana::widgets::skeletons::text_editor(wd, graph, scheme);
					editor_->multi_lines(false);
					editor_->set_accept([this](wchar_t ch)
					{
						auto str = editor_->text();
						auto pos = editor_->caret().x;
						if (ch == '\b')
						{
							if (pos > 0)
								str.erase(pos - 1, 1);
						}
						else
							str.insert(pos, 1, ch);

						return range_->check_value(to_utf8(str));
					});

					evt_agent_.reset(new event_agent(static_cast<nana::spinbox&>(wdg)));
					editor_->textbase().set_event_agent(evt_agent_.get());

					if (!range_)
						range_.reset(new range_numeric<int>(0, 100, 1));

					reset_text();

					API::tabstop(wd);
					API::eat_tabstop(wd, true);
					API::effects_edge_nimbus(wd, effects::edge_nimbus::active);
					API::effects_edge_nimbus(wd, effects::edge_nimbus::over);
					reset_text_area();
				}

				void detach()
				{
					delete editor_;
					editor_ = nullptr;
				}

				std::string value() const
				{
					return range_->value();
				}

				bool value(const ::std::string& value_str)
				{
					bool diff;
					if (!range_->value(value_str, diff))
						return false;

					if (diff)
						reset_text();
					return true;
				}

				void set_range(std::unique_ptr<range_interface> ptr)
				{
					range_.swap(ptr);

					reset_text();
				}

				void modifier(std::string&& prefix, std::string&& suffix)
				{
					modifier_.prefix = std::move(prefix);
					modifier_.suffix = std::move(suffix);

					if (editor_)
					{
						reset_text();
						API::update_window(editor_->window_handle());
					}
				}

				void draw_spins()
				{
					_m_draw_spins(buttons::none);
				}

				void render()
				{
					editor_->render(API::is_focus_ready(editor_->window_handle()));
					_m_draw_spins(spin_stated_);
				}

				::nana::widgets::skeletons::text_editor* editor() const
				{
					return editor_;
				}

				void mouse_wheel(bool upwards)
				{
					range_->spin(!upwards);
					reset_text();
				}

				bool mouse_button(const ::nana::arg_mouse& arg, bool pressed)
				{
					if (!pressed)
					{
						API::capture_window(editor_->window_handle(), false);
						timer_.stop();
						timer_.interval(600);
					}

					if (buttons::none != spin_stated_)
					{
						//Spins the value when mouse button is released
						if (pressed)
						{
							API::capture_window(editor_->window_handle(), true);
							range_->spin(buttons::increase == spin_stated_);
							reset_text();
							timer_.start();
						}
						else
							_m_draw_spins(spin_stated_);
						return true;
					}

					if (editor_->mouse_pressed(arg))
					{
						_m_draw_spins(buttons::none);
						return true;
					}

					return false;
				}

				bool mouse_move(bool left_button, const ::nana::point& pos)
				{
					if (editor_->mouse_move(left_button, pos))
					{
						editor_->reset_caret();
						render();
						return true;
					}

					auto btn = _m_where(pos);
					if (buttons::none != btn)
					{
						spin_stated_ = btn;
						_m_draw_spins(btn);
						return true;
					}
					else if (buttons::none != spin_stated_)
					{
						spin_stated_ = buttons::none;
						_m_draw_spins(buttons::none);
						return true;
					}

					return false;
				}

				void reset_text_area()
				{
					auto spins_r = _m_spins_area();
					if (spins_r.x == 0)
						editor_->text_area(rectangle{});
					else
						editor_->text_area({ 2, 2, graph_->width() - spins_r.width - 2, spins_r.height - 2 });
				}

				void reset_text()
				{
					if (!editor_)
						return;

					if (API::is_focus_ready(editor_->window_handle()))
						editor_->text(to_wstring(range_->value()));
					else
						editor_->text(to_wstring(modifier_.prefix + range_->value() + modifier_.suffix));

					_m_draw_spins(spin_stated_);
				}
			private:

				::nana::rectangle _m_spins_area() const
				{
					auto size = API::window_size(editor_->window_handle());
					if (size.width > 18)
						return{ static_cast<int>(size.width - 16), 0, 16, size.height };

					return{ 0, 0, size.width, size.height };
				}

				buttons _m_where(const ::nana::point& pos) const
				{
					auto spins_r = _m_spins_area();
					if (spins_r.is_hit(pos))
					{
						if (pos.y < spins_r.y + static_cast<int>(spins_r.height / 2))
							return buttons::increase;

						return buttons::decrease;
					}
					return buttons::none;
				}

				void _m_draw_spins(buttons spins)
				{
					auto estate = API::element_state(editor_->window_handle());

					auto spin_r0 = _m_spins_area();
					spin_r0.height /= 2;

					auto spin_r1 = spin_r0;
					spin_r1.y += static_cast<int>(spin_r0.height);
					spin_r1.height = _m_spins_area().height - spin_r0.height;

					::nana::color bgcolor{ 3, 65, 140 };
					facade<element::arrow> arrow;
					facade<element::button> button;

					auto spin_state = (buttons::increase == spins ? estate : element_state::normal);
					button.draw(*graph_, bgcolor, colors::white, spin_r0, spin_state);
					spin_r0.x += 5;
					arrow.draw(*graph_, bgcolor, colors::white, spin_r0, spin_state);

					spin_state = (buttons::decrease == spins ? estate : element_state::normal);
					button.draw(*graph_, bgcolor, colors::white, spin_r1, spin_state);
					spin_r1.x += 5;
					arrow.direction(direction::south);
					arrow.draw(*graph_, bgcolor, colors::white, spin_r1, spin_state);
				}
			private:
				::nana::paint::graphics * graph_{nullptr};
				::nana::widgets::skeletons::text_editor * editor_{nullptr};
				std::unique_ptr<event_agent> evt_agent_;
				buttons spin_stated_{ buttons::none };
				std::unique_ptr<range_interface> range_;
				::nana::timer timer_;

				struct modifiers
				{
					std::string prefix;
					std::string suffix;
				}modifier_;
			};

			//class drawer
			drawer::drawer()
				: impl_(new implementation)
			{}

			drawer::~drawer()
			{
				delete impl_;
			}

			implementation* drawer::impl() const
			{
				return impl_;
			}

			//Overrides drawer_trigger
			void drawer::attached(widget_reference wdg, graph_reference graph)
			{
				impl_->attach(wdg, graph);
			}

			void drawer::refresh(graph_reference)
			{
				impl_->render();
			}

			void drawer::focus(graph_reference, const arg_focus& arg)
			{
				impl_->reset_text();
				impl_->render();
				impl_->editor()->reset_caret();
				API::lazy_refresh();
			}

			void drawer::mouse_wheel(graph_reference, const arg_wheel& arg)
			{
				impl_->mouse_wheel(arg.upwards);
				impl_->editor()->reset_caret();
				API::lazy_refresh();
			}

			void drawer::mouse_down(graph_reference, const arg_mouse& arg)
			{
				if (impl_->mouse_button(arg, true))
					API::lazy_refresh();
			}

			void drawer::mouse_up(graph_reference, const arg_mouse& arg)
			{
				if (impl_->mouse_button(arg, false))
					API::lazy_refresh();
			}

			void drawer::mouse_move(graph_reference, const arg_mouse& arg)
			{
				if (impl_->mouse_move(arg.left_button, arg.pos))
					API::lazy_refresh();
			}

			void drawer::mouse_leave(graph_reference, const arg_mouse&)
			{
				impl_->render();
				API::lazy_refresh();
			}

			void drawer::key_press(graph_reference, const arg_keyboard& arg)
			{
				if (impl_->editor()->respond_key(arg))
				{
					impl_->editor()->reset_caret();
					impl_->draw_spins();
					API::lazy_refresh();
				}
			}

			void drawer::key_char(graph_reference, const arg_keyboard& arg)
			{
				if (impl_->editor()->respond_char(arg))
				{
					if (!impl_->value(to_utf8(impl_->editor()->text())))
						impl_->draw_spins();

					API::lazy_refresh();
				}
			}

			void drawer::resized(graph_reference graph, const arg_resized& arg)
			{
				impl_->reset_text_area();
				impl_->render();
				impl_->editor()->reset_caret();
				API::lazy_refresh();
			}
		}
	}//end namespace drawerbase

	spinbox::spinbox()
	{}

	spinbox::spinbox(window wd, bool visible)
	{
		this->create(wd, visible);
	}

	spinbox::spinbox(window wd, const nana::rectangle& r, bool visible)
	{
		this->create(wd, r, visible);
	}

	void spinbox::editable(bool accept)
	{
		internal_scope_guard lock;
		auto editor = get_drawer_trigger().impl()->editor();
		if (editor)
			editor->editable(accept);
	}

	bool spinbox::editable() const
	{
		auto editor = get_drawer_trigger().impl()->editor();
		return (editor ? editor->attr().editable : false);
	}

	void spinbox::range(int begin, int last, int step)
	{
		using namespace drawerbase::spinbox;
		get_drawer_trigger().impl()->set_range(std::unique_ptr<range_interface>(new range_numeric<int>(begin, last, step)));
		API::refresh_window(handle());
	}

	void spinbox::range(double begin, double last, double step)
	{
		using namespace drawerbase::spinbox;
		get_drawer_trigger().impl()->set_range(std::unique_ptr<range_interface>(new range_numeric<double>(begin, last, step)));
		API::refresh_window(handle());
	}

	void spinbox::range(std::initializer_list<std::string> steps_utf8)
	{
		using namespace drawerbase::spinbox;
		get_drawer_trigger().impl()->set_range(std::unique_ptr<range_interface>(new range_text(steps_utf8)));
		API::refresh_window(handle());
	}

	void spinbox::range(std::initializer_list<std::wstring> steps)
	{
		using namespace drawerbase::spinbox;
		get_drawer_trigger().impl()->set_range(std::unique_ptr<range_interface>(new range_text(steps)));
		API::refresh_window(handle());
	}

	::std::string spinbox::value() const
	{
		internal_scope_guard lock;
		if (handle())
			return get_drawer_trigger().impl()->value();
		return{};
	}

	void spinbox::value(const ::std::string& s)
	{
		internal_scope_guard lock;
		if (handle())
		{
			if (get_drawer_trigger().impl()->value(s))
				API::refresh_window(handle());
		}
	}

	int spinbox::to_int() const
	{
		return std::stoi(value());
	}

	double spinbox::to_double() const
	{
		return std::stod(value());
	}

	void spinbox::modifier(std::string prefix, std::string suffix)
	{
		get_drawer_trigger().impl()->modifier(std::move(prefix), std::move(suffix));
	}

	void spinbox::modifier(const std::wstring & prefix, const std::wstring& suffix)
	{
		modifier(to_utf8(prefix), to_utf8(suffix));
	}

	auto spinbox::_m_caption() const throw() -> native_string_type
	{
		internal_scope_guard lock;
		auto editor = get_drawer_trigger().impl()->editor();
		if (editor)
			return to_nstring(editor->text());
		return native_string_type();
	}

	void spinbox::_m_caption(native_string_type&& text)
	{
		internal_scope_guard lock;
		auto editor = get_drawer_trigger().impl()->editor();
		if (editor)
		{
			editor->text(to_wstring(text));
			API::refresh_window(*this);
		}
	}
}//end namespace nana
