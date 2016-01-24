/*
 *	A Combox Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/combox.cpp
 */

#include <nana/gui.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/element.hpp>
#include <nana/system/dataexch.hpp>
#include <nana/gui/widgets/float_listbox.hpp>
#include <nana/gui/widgets/skeletons/text_editor.hpp>
#include <nana/gui/widgets/skeletons/textbase_export_interface.hpp>

#include <iterator>

namespace nana
{
	arg_combox::arg_combox(combox& wdg): widget(wdg)
	{}

	namespace drawerbase
	{
		namespace combox
		{
			class event_agent
				: public widgets::skeletons::textbase_event_agent_interface
			{
			public:
				event_agent(::nana::combox& wdg)
					: widget_(wdg)
				{}

				void first_change() override{}	//empty, because combox does not have this event.

				void text_changed() override
				{
					widget_.events().text_changed.emit(::nana::arg_combox{ widget_ });
				}
			private:
				::nana::combox & widget_;
			};

			struct item
				: public float_listbox::item_interface
			{
			public:
				std::shared_ptr<nana::detail::key_interface> key;

				nana::paint::image	item_image;
				std::string		item_text;
				mutable std::shared_ptr<nana::any>	any_ptr;

				item(std::shared_ptr<nana::detail::key_interface> && kv)
					: key(std::move(kv))
				{
				}

				item(std::string&& s)
					: item_text(std::move(s))
				{}
			private:
				//implement item_interface methods
				const nana::paint::image & image() const override
				{
					return item_image;
				}

				const char* text() const override
				{
					return item_text.data();
				}
			};

			class drawer_impl
			{
			public:
				using graph_reference = paint::graphics&;
				using widget_reference = widget&;

				enum class parts{none, text, push_button};

				drawer_impl()
				{
					state_.focused = false;
					state_.button_state = element_state::normal;
					state_.pointer_where = parts::none;
					state_.lister = nullptr;
				}

				void renderer(drawerbase::float_listbox::item_renderer* ir)
				{
					item_renderer_ = ir;
				}

				void attached(widget_reference wd, graph_reference graph)
				{
					widget_ = static_cast< ::nana::combox*>(&wd);

					auto scheme = dynamic_cast< ::nana::widgets::skeletons::text_editor_scheme*>(API::dev::get_scheme(wd));
					editor_ = new widgets::skeletons::text_editor(widget_->handle(), graph, scheme);
					editor_->multi_lines(false);
					editable(false);
					graph_ = &graph;

					evt_agent_.reset(new event_agent{ static_cast<nana::combox&>(wd) });
					editor_->textbase().set_event_agent(evt_agent_.get());
				}

				void detached()
				{
					delete editor_;
					editor_ = nullptr;
					graph_ = nullptr;
				}

				void insert(std::string&& text)
				{
					items_.emplace_back(std::make_shared<item>(std::move(text)));
					API::refresh_window(widget_->handle());
				}

				nana::any * anyobj(std::size_t pos, bool allocate_if_empty) const
				{
					if(pos >= items_.size())
						return nullptr;

					auto & any_ptr = items_[pos]->any_ptr;
					if (allocate_if_empty && (nullptr == any_ptr))
						any_ptr = std::make_shared<nana::any>();
					return any_ptr.get();
				}

				void text_area(const nana::size& s)
				{
					nana::rectangle r(2, 2, s.width > 19 ? s.width - 19 : 0, s.height > 4 ? s.height - 4 : 0);
					if(image_enabled_)
					{
						unsigned place = image_pixels_ + 2;
						r.x += place;
						if(r.width > place)	r.width -= place;
					}
					editor_->text_area(r);
				}

				widgets::skeletons::text_editor * editor() const
				{
					return editor_;
				}

				widget* widget_ptr() const
				{
					return widget_;
				}

				void clear()
				{
					items_.clear();
					module_.items.clear();
					module_.index = nana::npos;
				}

				void editable(bool enb)
				{
					if(editor_)
					{
						editor_->editable(enb);

						if (!enb)
						{
							editor_->ext_renderer().background = [this](graph_reference graph, const ::nana::rectangle&, const ::nana::color&)
							{
								auto clr_from = colors::button_face_shadow_start;
								auto clr_to = colors::button_face_shadow_end;

								int pare_off_px = 1;
								if (element_state::pressed == state_.button_state)
								{
									pare_off_px = 2;
									std::swap(clr_from, clr_to);
								}

								graph.gradual_rectangle(::nana::rectangle(graph.size()).pare_off(pare_off_px), clr_from, clr_to, true);
							};
						}
						else
							editor_->ext_renderer().background = nullptr;

						editor_->enable_background(enb);
						editor_->enable_background_counterpart(!enb);
						API::refresh_window(widget_->handle());
					}
				}

				bool editable() const
				{
					return (editor_ && editor_->attr().editable);
				}

				bool calc_where(graph_reference graph, int x, int y)
				{
					auto new_where = parts::none;
					if(1 < x && x < static_cast<int>(graph.width()) - 2 && 1 < y && y < static_cast<int>(graph.height()) - 2)
					{
						if((editor_->attr().editable == false) || (static_cast<int>(graph.width()) - 22 <= x))
							new_where = parts::push_button;
						else
							new_where = parts::text;
					}

					if (new_where == state_.pointer_where)
						return false;

					state_.pointer_where = new_where;
					return true;
				}

				void set_button_state(element_state state, bool reset_where)
				{
					state_.button_state = state;
					if (reset_where)
						state_.pointer_where = parts::none;
				}

				void set_focused(bool f)
				{
					if(editor_)
					{
						state_.focused = f;
						if(editor_->attr().editable)
							editor_->select(f);
					}
				}

				bool has_lister() const
				{
					return (state_.lister != nullptr);
				}

				void open_lister_if_push_button_positioned()
				{
					if((nullptr == state_.lister) && !items_.empty() && (parts::push_button == state_.pointer_where))
					{
						module_.items.clear();
						std::copy(items_.cbegin(), items_.cend(), std::back_inserter(module_.items));
						state_.lister = &form_loader<nana::float_listbox, false>()(widget_->handle(), nana::rectangle(0, widget_->size().height, widget_->size().width, 10), true);
						state_.lister->renderer(item_renderer_);
						state_.lister->set_module(module_, image_pixels_);
						state_.item_index_before_selection = module_.index;
						//The lister window closes by itself. I just take care about the destroy event.
						//The event should be destroy rather than unload. Because the unload event is invoked while
						//the lister is not closed, if popuping a message box, the lister will cover the message box.
						state_.lister->events().destroy.connect_unignorable([this]
						{
							_m_lister_close_sig();
						});
					}
				}

				void scroll_items(bool upwards)
				{
					if(state_.lister)
						state_.lister->scroll_items(upwards);
				}

				void move_items(bool upwards, bool circle)
				{
					if (state_.lister)
					{
						state_.lister->move_items(upwards, circle);
						return;
					}

					auto pos = module_.index;
					if (upwards)
					{
						if (pos && (pos < items_.size()))
							--pos;
						else if (circle)
							pos = items_.size() - 1;
					}
					else
					{
						if ((pos + 1) < items_.size())
							++pos;
						else if (circle)
							pos = 0;
					}

					if (pos != module_.index)
						option(pos, false);
				}

				void draw()
				{
					bool enb = widget_->enabled();
					if(editor_)
					{
						text_area(widget_->size());
						editor_->render(state_.focused);
					}
					_m_draw_push_button(enb);
					_m_draw_image();
				}

				std::size_t the_number_of_options() const
				{
					return items_.size();
				}

				std::size_t option() const
				{
					return (module_.index < items_.size() ? module_.index : nana::npos);
				}

				void option(std::size_t index, bool ignore_condition)
				{
					if(items_.size() <= index)
						return;

					std::size_t old_index = module_.index;
					module_.index = index;

					if(nullptr == widget_)
						return;

					//Test if the current item or text is different from selected.
					if(ignore_condition || (old_index != index) || (items_[index]->item_text != widget_->caption()))
					{
						auto pos = API::cursor_position();
						API::calc_window_point(widget_->handle(), pos);
						if (calc_where(*graph_, pos.x, pos.y))
							state_.button_state = element_state::normal;

						editor_->text(::nana::charset(items_[index]->item_text, ::nana::unicode::utf8));
						_m_draw_push_button(widget_->enabled());
						_m_draw_image();

						widget_->events().selected.emit(::nana::arg_combox(*widget_));
					}
				}

				std::size_t at_key(std::shared_ptr<nana::detail::key_interface>&& p)
				{
					std::size_t pos = 0;
					for (auto & m : items_)
					{
						if (m->key && detail::pred_equal_by_less(m->key.get(), p.get()))
							return pos;
						++pos;
					}

					pos = items_.size();
					items_.emplace_back(std::make_shared<item>(std::move(p)));

					//Redraw, because the state of push button is changed when a first new item is created.
					if (0 == pos)
						API::refresh_window(*widget_);

					return pos;
				}

				void erase(detail::key_interface * kv)
				{
					std::size_t pos = 0;
					for (auto & m : items_)
					{
						if (m->key && detail::pred_equal_by_less(m->key.get(), kv))
						{
							erase(pos);
							return;
						}
						++pos;
					}
				}

				item& at(std::size_t pos)
				{
					return *items_.at(pos);
				}

				const item& at(std::size_t pos) const
				{
					return *items_.at(pos);
				}

				void erase(std::size_t pos)
				{
					if (pos >= items_.size())
						return;

					if (pos == module_.index)
					{
						module_.index = ::nana::npos;
						this->widget_->caption("");
					}
					else if ((::nana::npos != module_.index) && (pos < module_.index))
						--module_.index;

					items_.erase(items_.begin() + pos);

					//Redraw, because the state of push button is changed when the last item is removed.
					if (items_.empty())
						API::refresh_window(*widget_);
				}

				void image(std::size_t pos, const nana::paint::image& img)
				{
					if (pos < items_.size())
					{
						items_[pos]->item_image = img;
						if ((false == image_enabled_) && img)
						{
							image_enabled_ = true;
							draw();
						}
					}
				}

				bool image_pixels(unsigned px)
				{
					if (image_pixels_ == px)
						return false;

					image_pixels_ = px;
					return true;
				}
			private:
				void _m_lister_close_sig()
				{
					state_.lister = nullptr;	//The lister closes by itself.
					if ((module_.index != nana::npos) && (module_.index != state_.item_index_before_selection))
					{
						option(module_.index, true);
						API::update_window(*widget_);
					}
					else
					{
						//Redraw the widget even though the index has not been changed,
						//because the push button should be updated due to the state
						//changed from pressed to normal/hovered.
						API::refresh_window(*widget_);
					}
				}

				void _m_draw_push_button(bool enabled)
				{
					::nana::rectangle r{graph_->size()};
					r.x = r.right() - 16;
					r.y = 1;
					r.width = 16;
					r.height -= 2;

					auto estate = state_.button_state;
					if (enabled && !items_.empty())
					{
						if (has_lister() || (element_state::pressed == estate && state_.pointer_where == parts::push_button))
							estate = element_state::pressed;
					}
					else
						estate = element_state::disabled;

					facade<element::button> button;
					button.draw(*graph_, ::nana::color{ 3, 65, 140 }, colors::white, r, estate);

					facade<element::arrow> arrow("solid_triangle");
					arrow.direction(::nana::direction::south);

					r.y += (r.height / 2) - 7;
					r.width = r.height = 16;
					arrow.draw(*graph_, {}, colors::white, r, element_state::normal);
				}

				void _m_draw_image()
				{
					if(items_.size() <= module_.index)
						return;

					auto & img = items_[module_.index]->item_image;

					if(img.empty())
						return;

					unsigned vpix = editor_->line_height();
					nana::size imgsz = img.size();
					if(imgsz.width > image_pixels_)
					{
						unsigned new_h = image_pixels_ * imgsz.height / imgsz.width;
						if(new_h > vpix)
						{
							imgsz.width = vpix * imgsz.width / imgsz.height;
							imgsz.height = vpix;
						}
						else
						{
							imgsz.width = image_pixels_;
							imgsz.height = new_h;
						}
					}
					else if(imgsz.height > vpix)
					{
						unsigned new_w = vpix * imgsz.width / imgsz.height;
						if(new_w > image_pixels_)
						{
							imgsz.height = image_pixels_ * imgsz.height / imgsz.width;
							imgsz.width = image_pixels_;
						}
						else
						{
							imgsz.height = vpix;
							imgsz.width = new_w;
						}
					}

					nana::point pos((image_pixels_ - imgsz.width) / 2 + 2, (vpix - imgsz.height) / 2 + 2);
					img.stretch(::nana::rectangle{ img.size() }, *graph_, nana::rectangle(pos, imgsz));
				}
			private:
				std::vector<std::shared_ptr<item>> items_;
				nana::float_listbox::module_type module_;
				::nana::combox * widget_{ nullptr };
				nana::paint::graphics * graph_{ nullptr };
				drawerbase::float_listbox::item_renderer* item_renderer_{ nullptr };

				bool image_enabled_{ false };
				unsigned image_pixels_{ 16 };
				widgets::skeletons::text_editor * editor_{ nullptr };
				std::unique_ptr<event_agent> evt_agent_;
				struct state_type
				{
					bool	focused;
					element_state button_state;
					parts	pointer_where;

					nana::float_listbox * lister;
					std::size_t	item_index_before_selection;
				}state_;
			};


			//class trigger
				trigger::trigger()
					: drawer_(new drawer_impl)
				{}

				trigger::~trigger()
				{
					delete drawer_;
				}

				drawer_impl& trigger::get_drawer_impl()
				{
					return *drawer_;
				}

				const drawer_impl& trigger::get_drawer_impl() const
				{
					return *drawer_;
				}

				void trigger::attached(widget_reference wdg, graph_reference graph)
				{
					wdg.bgcolor(colors::white);
					drawer_->attached(wdg, graph);

					API::effects_edge_nimbus(wdg, effects::edge_nimbus::active);
					API::effects_edge_nimbus(wdg, effects::edge_nimbus::over);
				}

				void trigger::detached()
				{
					drawer_->detached();
				}

				void trigger::refresh(graph_reference)
				{
					drawer_->draw();
				}

				void trigger::focus(graph_reference, const arg_focus& arg)
				{
					drawer_->set_focused(arg.getting);
					if(drawer_->widget_ptr()->enabled())
					{
						drawer_->draw();
						drawer_->editor()->reset_caret();
						API::lazy_refresh();
					}
				}

				void trigger::mouse_enter(graph_reference, const arg_mouse&)
				{
					drawer_->set_button_state(element_state::hovered, true);
					if(drawer_->widget_ptr()->enabled())
					{
						drawer_->draw();
						API::lazy_refresh();
					}
				}

				void trigger::mouse_leave(graph_reference, const arg_mouse&)
				{
					drawer_->set_button_state(element_state::normal, true);
					drawer_->editor()->mouse_enter(false);
					if(drawer_->widget_ptr()->enabled())
					{
						drawer_->draw();
						API::lazy_refresh();
					}
				}

				void trigger::mouse_down(graph_reference graph, const arg_mouse& arg)
				{
					//drawer_->set_mouse_press(true);
					drawer_->set_button_state(element_state::pressed, false);
					if(drawer_->widget_ptr()->enabled())
					{
						auto * editor = drawer_->editor();
						if (!editor->mouse_pressed(arg))
							drawer_->open_lister_if_push_button_positioned();

						drawer_->draw();
						if(editor->attr().editable)
							editor->reset_caret();

						API::lazy_refresh();
					}
				}

				void trigger::mouse_up(graph_reference graph, const arg_mouse& arg)
				{
					if (drawer_->widget_ptr()->enabled() && !drawer_->has_lister())
					{
						drawer_->editor()->mouse_pressed(arg);
						drawer_->set_button_state(element_state::hovered, false);
						drawer_->draw();
						API::lazy_refresh();
					}
				}

				void trigger::mouse_move(graph_reference graph, const arg_mouse& arg)
				{
					if(drawer_->widget_ptr()->enabled())
					{
						bool redraw = drawer_->calc_where(graph, arg.pos.x, arg.pos.y);
						redraw |= drawer_->editor()->mouse_move(arg.left_button, arg.pos);

						if(redraw)
						{
							drawer_->draw();
							drawer_->editor()->reset_caret();
							API::lazy_refresh();
						}
					}
				}

				void trigger::mouse_wheel(graph_reference graph, const arg_wheel& arg)
				{
					if(drawer_->widget_ptr()->enabled())
					{
						if(drawer_->has_lister())
							drawer_->scroll_items(arg.upwards);
						else
							drawer_->move_items(arg.upwards, false);
					}
				}

				void trigger::key_press(graph_reference, const arg_keyboard& arg)
				{
					if(!drawer_->widget_ptr()->enabled())
						return;

					bool call_other_keys = false;
					if(drawer_->editable())
					{
						bool is_move_up = false;
						switch(arg.key)
						{
						case keyboard::os_arrow_left:
						case keyboard::os_arrow_right:
							drawer_->editor()->respond_key(arg);
							drawer_->editor()->reset_caret();
							break;
						case keyboard::os_arrow_up:
							is_move_up = true;
						case keyboard::os_arrow_down:
							drawer_->move_items(is_move_up, true);
							break;
						default:
							call_other_keys = true;
						}
					}
					else
					{
						bool is_move_up = false;
						switch(arg.key)
						{
						case keyboard::os_arrow_left:
						case keyboard::os_arrow_up:
							is_move_up = true;
						case keyboard::os_arrow_right:
						case keyboard::os_arrow_down:
							drawer_->move_items(is_move_up, true);
							break;
						default:
							call_other_keys = true;
						}
					}
					if (call_other_keys)
						drawer_->editor()->respond_key(arg);

					API::lazy_refresh();
				}

				void trigger::key_char(graph_reference graph, const arg_keyboard& arg)
				{
					if (drawer_->editor()->respond_char(arg))
						API::lazy_refresh();
				}
			//end class trigger

			//class item_proxy
				item_proxy::item_proxy(drawer_impl* impl, std::size_t pos)
					:	impl_(impl),
						pos_(pos)
				{}

				item_proxy& item_proxy::text(const ::std::string& s)
				{
					throw_not_utf8(s);
					impl_->at(pos_).item_text = s;
					return *this;
				}

				::std::string item_proxy::text() const
				{
					return impl_->at(pos_).item_text;
				}

				bool	item_proxy::selected() const
				{
					return pos_ == impl_->option();
				}

				item_proxy&	item_proxy::select()
				{
					impl_->option(pos_, false);
					return *this;
				}

				item_proxy& item_proxy::icon(const nana::paint::image& img)
				{
					impl_->image(pos_, img);
					if (pos_ == impl_->option())
						API::refresh_window(impl_->widget_ptr()->handle());
					return *this;
				}

				nana::paint::image item_proxy::icon() const
				{
					return impl_->at(pos_).item_image;
				}

				/// Behavior of Iterator's value_type
				bool item_proxy::operator == (const ::std::string& s) const
				{
					if (pos_ == nana::npos)
						return false;
					return (impl_->at(pos_).item_text ==s);
				}

				bool item_proxy::operator == (const char * s) const
				{
					if (pos_ == nana::npos)
						return false;
					return (impl_->at(pos_).item_text == s);
				}


				/// Behavior of Iterator
				item_proxy & item_proxy::operator=(const item_proxy& r)
				{
					if (this != &r)
					{
						impl_ = r.impl_;
						pos_ = r.pos_;
					}
					return *this;
				}

				/// Behavior of Iterator
				item_proxy & item_proxy::operator++()
				{
					if (nana::npos != pos_)
					{
						if (++pos_ == impl_->the_number_of_options())
							pos_ = nana::npos;
					}
					return *this;
				}

				/// Behavior of Iterator
				item_proxy	item_proxy::operator++(int)
				{
					if (pos_ == nana::npos)
						return *this;

					item_proxy tmp = *this;
					if (++pos_ == impl_->the_number_of_options())
						pos_ = nana::npos;

					return tmp;
				}

				/// Behavior of Iterator
				item_proxy& item_proxy::operator*()
				{
					return *this;
				}

				/// Behavior of Iterator
				const item_proxy& item_proxy::operator*() const
				{
					return *this;
				}

				/// Behavior of Iterator
				item_proxy* item_proxy::operator->()
				{
					return this;
				}

				/// Behavior of Iterator
				const item_proxy* item_proxy::operator->() const
				{
					return this;
				}

				/// Behavior of Iterator
				bool item_proxy::operator==(const item_proxy& r) const
				{
					return (impl_ == r.impl_ && pos_ == r.pos_);
				}

				/// Behavior of Iterator
				bool item_proxy::operator!=(const item_proxy& r) const
				{
					return ! this->operator==(r);
				}

				nana::any * item_proxy::_m_anyobj(bool alloc_if_empty) const
				{
					return impl_->anyobj(pos_, alloc_if_empty);
				}
			//end class item_proxy
		}
	}//end namespace drawerbase

	//class combox
		combox::combox(){}

		combox::combox(window wd, bool visible)
		{
			create(wd, rectangle(), visible);
		}

		combox::combox(window wd, std::string text, bool visible)
		{
			throw_not_utf8(text);
			create(wd, rectangle(), visible);
			caption(std::move(text));
		}

		combox::combox(window wd, const char* text, bool visible)
			: combox(wd, std::string(text), visible)
		{
		}

		combox::combox(window wd, const nana::rectangle& r, bool visible)
		{
			create(wd, r, visible);
		}

		void combox::clear()
		{
			internal_scope_guard lock;
			_m_impl().clear();
			API::refresh_window(handle());
		}

		void combox::editable(bool eb)
		{
			internal_scope_guard lock;
			_m_impl().editable(eb);
		}

		bool combox::editable() const
		{
			internal_scope_guard lock;
			return _m_impl().editable();
		}

		void combox::set_accept(std::function<bool(wchar_t)> pred)
		{
			internal_scope_guard lock;
			auto editor = _m_impl().editor();
			if(editor)
				editor->set_accept(std::move(pred));
		}

		combox& combox::push_back(std::string text)
		{
			internal_scope_guard lock;
			_m_impl().insert(std::move(text));
			return *this;
		}

		std::size_t combox::the_number_of_options() const
		{
			internal_scope_guard lock;
			return _m_impl().the_number_of_options();
		}

		std::size_t combox::option() const
		{
			internal_scope_guard lock;
			return _m_impl().option();
		}

		void combox::option(std::size_t pos)
		{
			internal_scope_guard lock;
			_m_impl().option(pos, false);
			API::update_window(handle());
		}

		::std::string combox::text(std::size_t pos) const
		{
			internal_scope_guard lock;
			return _m_impl().at(pos).item_text;
		}

		void combox::erase(std::size_t pos)
		{
			internal_scope_guard lock;
			_m_impl().erase(pos);
		}

		void combox::renderer(item_renderer* ir)
		{
			internal_scope_guard lock;
			_m_impl().renderer(ir);
		}

		void combox::image(std::size_t i, const nana::paint::image& img)
		{
			internal_scope_guard lock;
			if(empty()) return;

			auto & impl = _m_impl();
			impl.image(i, img);
			if(i == impl.option())
				API::refresh_window(*this);
		}

		nana::paint::image combox::image(std::size_t pos) const
		{
			internal_scope_guard lock;
			return _m_impl().at(pos).item_image;
		}

		void combox::image_pixels(unsigned px)
		{
			internal_scope_guard lock;
			if (_m_impl().image_pixels(px))
				API::refresh_window(*this);
		}

		auto combox::_m_caption() const throw() -> native_string_type
		{
			internal_scope_guard lock;
			auto editor = _m_impl().editor();
			if (editor)
				return to_nstring(editor->text());
			return native_string_type();
		}

		void combox::_m_caption(native_string_type&& str)
		{
			internal_scope_guard lock;

			auto editor = _m_impl().editor();
			if (editor)
				editor->text(to_wstring(str));

			API::refresh_window(*this);
		}

		nana::any * combox::_m_anyobj(std::size_t pos, bool alloc_if_empty) const
		{
			internal_scope_guard lock;
			return _m_impl().anyobj(pos, alloc_if_empty);
		}

		auto combox::_m_at_key(std::shared_ptr<nana::detail::key_interface>&& p) -> item_proxy
		{
			internal_scope_guard lock;
			auto & impl = _m_impl();
			return item_proxy(&impl, impl.at_key(std::move(p)));
		}

		void combox::_m_erase(nana::detail::key_interface* p)
		{
			internal_scope_guard lock;
			_m_impl().erase(p);
		}

		drawerbase::combox::drawer_impl & combox::_m_impl()
		{
			return get_drawer_trigger().get_drawer_impl();
		}

		const drawerbase::combox::drawer_impl& combox::_m_impl() const
		{
			return get_drawer_trigger().get_drawer_impl();
		}
	//end class combox
}
