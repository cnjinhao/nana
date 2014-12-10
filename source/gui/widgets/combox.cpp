/*
 *	A Combox Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/combox.cpp
 */

#include <nana/gui/wvl.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/paint/gadget.hpp>
#include <nana/system/dataexch.hpp>
#include <nana/gui/widgets/float_listbox.hpp>
#include <nana/gui/widgets/skeletons/text_editor.hpp>

namespace nana
{
	namespace drawerbase
	{
		namespace combox
		{
			struct item
				: public float_listbox::item_interface
			{
			public:
				std::shared_ptr<nana::detail::key_interface> key;

				nana::paint::image	item_image;
				nana::string		item_text;
				mutable std::shared_ptr<nana::any>	any_ptr;

				item(std::shared_ptr<nana::detail::key_interface> && kv)
					: key(std::move(kv))
				{
				}

				item(const nana::string& s)
					: item_text(s)
				{}
			private:
				//implement item_interface methods
				const nana::paint::image & image() const override
				{
					return item_image;
				}

				const nana::char_t* text() const override
				{
					return item_text.data();
				}
			};

			class drawer_impl
			{
			public:
				typedef nana::paint::graphics & graph_reference;
				typedef widget	& widget_reference;

				enum class where_t{unknown, text, push_button};
				enum class state_t{none, mouse_over, pressed};

				drawer_impl()
				{
					state_.focused = false;
					state_.state = state_t::none;
					state_.pointer_where = where_t::unknown;
					state_.lister = nullptr;
				}

				~drawer_impl()
				{
					clear();
				}

				void renderer(drawerbase::float_listbox::item_renderer* ir)
				{
					item_renderer_ = ir;
				}

				void attached(widget_reference wd, graph_reference graph)
				{
					widget_ = static_cast< ::nana::combox*>(&wd);
					editor_ = new widgets::skeletons::text_editor(widget_->handle(), graph);
					editor_->border_renderer([this](graph_reference graph, nana::color_t bgcolor){
						draw_border(graph, bgcolor);
					});
					editor_->multi_lines(false);
					editable(false);
					graph_ = &graph;
				}

				void detached()
				{
					delete editor_;
					editor_ = nullptr;
					graph_ = nullptr;
				}

				void insert(const nana::string& text)
				{
					items_.emplace_back(std::make_shared<item>(text));
					API::refresh_window(widget_->handle());
				}

				where_t get_where() const
				{
					return state_.pointer_where;
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

						if(enb)
							editor_->ext_renderer().background = nullptr;
						else
							editor_->ext_renderer().background = std::bind(&drawer_impl::_m_draw_background, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

						editor_->enable_background(enb);
						editor_->enable_background_counterpart(!enb);
						API::refresh_window(widget_->handle());
					}
				}

				bool editable() const
				{
					return (editor_ ? editor_->attr().editable : false);
				}

				bool calc_where(graph_reference graph, int x, int y)
				{
					auto new_where = where_t::unknown;

					if(1 < x && x < static_cast<int>(graph.width()) - 2 && 1 < y && y < static_cast<int>(graph.height()) - 2)
					{
						if((editor_->attr().editable == false) || (static_cast<int>(graph.width()) - 22 <= x))
							new_where = where_t::push_button;
						else
							new_where = where_t::text;
					}

					if(new_where != state_.pointer_where)
					{
						state_.pointer_where = new_where;
						return true;
					}
					return false;
				}

				void set_mouse_over(bool mo)
				{
					state_.state = mo ? state_t::mouse_over : state_t::none;
					state_.pointer_where = where_t::unknown;
				}

				void set_mouse_press(bool mp)
				{
					state_.state = (mp ? state_t::pressed : state_t::mouse_over);
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

				void open_lister()
				{
					if((nullptr == state_.lister) && !items_.empty())
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
						state_.lister->events().destroy.connect([this]
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
					if(nullptr == state_.lister)
					{
						std::size_t orig_i = module_.index;
						if(upwards)
						{
							if(module_.index && (module_.index < items_.size()))
								-- module_.index;
							else if(circle)
								module_.index = items_.size() - 1;
						}
						else
						{
							if((module_.index + 1) < items_.size())
								++ module_.index;
							else if(circle)
								module_.index = 0;
						}

						if(orig_i != module_.index)
							option(module_.index, false);
					}
					else
						state_.lister->move_items(upwards, circle);
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

				void draw_border(graph_reference graph, nana::color_t bgcolor)
				{
					graph.rectangle((state_.focused ? 0x0595E2 : 0x999A9E), false);
					nana::rectangle r(graph.size());
					graph.rectangle(r.pare_off(1), bgcolor, false);
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
						if(calc_where(*graph_, pos.x, pos.y))
							state_.state = state_t::none;

						editor_->text(items_[index]->item_text);
						_m_draw_push_button(widget_->enabled());
						_m_draw_image();

						//Yes, it's safe to static_cast here!
						widget_->events().selected.emit(::nana::arg_combox(*widget_));
					}
				}

				std::size_t at_key(std::shared_ptr<nana::detail::key_interface>&& p)
				{
					std::size_t pos = 0;
					for (auto & m : items_)
					{
						if (m->key && nana::detail::pred_equal_by_less(m->key.get(), p.get()))
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

				void erase(nana::detail::key_interface * kv)
				{
					for (auto i = items_.begin(); i != items_.end(); ++i)
					{
						if ((*i)->key && nana::detail::pred_equal_by_less((*i)->key.get(), kv))
						{
							std::size_t pos = i - items_.begin();
							this->erase(pos);
							return;
						}
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
					if (pos < items_.size())
					{
						if (pos == module_.index)
						{
							module_.index = nana::npos;
							this->widget_->caption(L"");
						}
						else if ((nana::npos != module_.index) && (pos < module_.index))
							--module_.index;

						items_.erase(items_.begin() + pos);

						//Redraw, because the state of push button is changed when the last item is created.
						if (items_.empty())
							API::refresh_window(*widget_);
					}
				}

				void text(nana::string&& str)
				{
					if (editor_)
						editor_->text(std::move(str));
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
					if(image_pixels_ != px)
					{
						image_pixels_ = px;
						return true;
					}
					return false;
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

				void _m_draw_background(graph_reference graph, const nana::rectangle&, nana::color_t)
				{
					nana::rectangle r(graph.size());
					nana::color_t color_start = color::button_face_shadow_start;
					nana::color_t color_end = color::button_face_shadow_end;
					if(state_.state == state_t::pressed)
					{
						r.pare_off(2);
						std::swap(color_start, color_end);
					}
					else
						r.pare_off(1);

					graph.shadow_rectangle(r, color_start, color_end, true);
				}

				void _m_draw_push_button(bool enabled)
				{
					using namespace nana::paint;

					if (nullptr == graph_) return;

					int left = graph_->width() - 17;
					int right = left + 16;
					int top = 1;
					int bottom = graph_->height() - 2;
					int mid = top + (bottom - top) * 5 / 18;

					nana::color_t topcol, topcol_ln, botcol, botcol_ln;
					nana::color_t arrow_color;
					if (enabled && items_.size())
					{
						arrow_color = 0xFFFFFF;
						double percent = 1;
						if (has_lister() || (state_.state == state_t::pressed && state_.pointer_where == where_t::push_button))
							percent = 0.8;
						else if (state_.state == state_t::mouse_over)
							percent = 0.9;

						topcol_ln = graphics::mix(0x3F476C, 0xFFFFFF, percent);
						botcol_ln = graphics::mix(0x031141, 0xFFFFFF, percent);
						topcol = graphics::mix(0x3F83B4, 0xFFFFFF, percent);
						botcol = graphics::mix(0x0C4A95, 0xFFFFFF, percent);
					}
					else
					{
						arrow_color = 0xFFFFFF;
						topcol_ln = 0x7F7F7F;
						botcol_ln = 0x505050;
						topcol = 0xC3C3C3;
						botcol = 0xA0A0A0;
					}

					graph_->line(left, top, left, mid, topcol_ln);
					graph_->line(right - 1, top, right - 1, mid, topcol_ln);

					graph_->line(left, mid + 1, left, bottom, botcol_ln);
					graph_->line(right - 1, mid + 1, right - 1, bottom, botcol_ln);

					graph_->rectangle(left + 1, top, right - left - 2, mid - top + 1, topcol, true);
					graph_->rectangle(left + 1, mid + 1, right - left - 2, bottom - mid, botcol, true);

					gadget::arrow_16_pixels(*graph_, left, top + ((bottom - top) / 2) - 7, arrow_color, 1, gadget::directions::to_south);
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
					img.stretch(img.size(), *graph_, nana::rectangle(pos, imgsz));
				}
			private:
				std::vector<std::shared_ptr<item> > items_;
				nana::float_listbox::module_type module_;
				::nana::combox * widget_ = nullptr;
				nana::paint::graphics * graph_ = nullptr;
				drawerbase::float_listbox::item_renderer* item_renderer_ = nullptr;

				bool image_enabled_ = false;
				unsigned image_pixels_ = 16;
				widgets::skeletons::text_editor * editor_ = nullptr;

				struct state_type
				{
					bool	focused;
					state_t	state;
					where_t	pointer_where;

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
					drawer_ = nullptr;
				}

				void trigger::set_accept(std::function<bool(nana::char_t)>&& pred)
				{
					pred_acceptive_ = std::move(pred);
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
					wdg.background(0xFFFFFF);
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
					drawer_->set_mouse_over(true);
					if(drawer_->widget_ptr()->enabled())
					{
						drawer_->draw();
						API::lazy_refresh();
					}
				}

				void trigger::mouse_leave(graph_reference, const arg_mouse&)
				{
					drawer_->set_mouse_over(false);
					drawer_->editor()->mouse_enter(false);
					if(drawer_->widget_ptr()->enabled())
					{
						drawer_->draw();
						API::lazy_refresh();
					}
				}

				void trigger::mouse_down(graph_reference graph, const arg_mouse& arg)
				{
					drawer_->set_mouse_press(true);
					if(drawer_->widget_ptr()->enabled())
					{
						auto * editor = drawer_->editor();
						if(false == editor->mouse_down(arg.left_button, arg.pos))
							if(drawer_impl::where_t::push_button == drawer_->get_where())
								drawer_->open_lister();

						drawer_->draw();
						if(editor->attr().editable)
							editor->reset_caret();

						API::lazy_refresh();
					}
				}

				void trigger::mouse_up(graph_reference graph, const arg_mouse& arg)
				{
					if(drawer_->widget_ptr()->enabled())
					{
						if(false == drawer_->has_lister())
						{
							drawer_->editor()->mouse_up(arg.left_button, arg.pos);
							drawer_->set_mouse_press(false);
							drawer_->draw();
							API::lazy_refresh();
						}
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
					if(false == drawer_->widget_ptr()->enabled())
						return;

					if(drawer_->editable())
					{
						bool is_move_up = false;
						switch(arg.key)
						{
						case keyboard::os_arrow_left:
						case keyboard::os_arrow_right:
							drawer_->editor()->move(arg.key);
							drawer_->editor()->reset_caret();
							break;
						case keyboard::os_arrow_up:
							is_move_up = true;
						case keyboard::os_arrow_down:
							drawer_->move_items(is_move_up, true);
							break;
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
						}
					}
					API::lazy_refresh();
				}

				void trigger::key_char(graph_reference graph, const arg_keyboard& arg)
				{
					bool enterable = drawer_->widget_ptr()->enabled() && (!pred_acceptive_ || pred_acceptive_(arg.key));
					if (drawer_->editor()->respone_keyboard(arg.key, enterable))
						API::lazy_refresh();
				}
			//end class trigger

			//class item_proxy
				item_proxy::item_proxy(drawer_impl* impl, std::size_t pos)
					:	impl_(impl),
						pos_(pos)
				{}

				item_proxy& item_proxy::text(const nana::string& s)
				{
					impl_->at(pos_).item_text = s;
					return *this;
				}

				nana::string item_proxy::text() const
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
				bool item_proxy::operator == (const nana::string& s) const
				{
					if (pos_ == nana::npos)
						return false;
					return (impl_->at(pos_).item_text ==s);
				}

				bool item_proxy::operator == (const char * s) const
				{
					if (pos_ == nana::npos)
						return false;
					return (impl_->at(pos_).item_text == static_cast<nana::string>(nana::charset(s)));
				}

				bool item_proxy::operator == (const wchar_t * s) const
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
					if (nana::npos == pos_)
						return *this;

					if (++pos_ == impl_->the_number_of_options())
						pos_ = nana::npos;

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

		combox::combox(window wd, const nana::string& text, bool visible)
		{
			create(wd, rectangle(), visible);
			caption(text);
		}

		combox::combox(window wd, const nana::char_t* text, bool visible)
		{
			create(wd, rectangle(), visible);
			caption(text);
		}

		combox::combox(window wd, const nana::rectangle& r, bool visible)
		{
			create(wd, r, visible);
		}

		void combox::clear()
		{
			internal_scope_guard sg;
			get_drawer_trigger().get_drawer_impl().clear();
			API::refresh_window(handle());
		}

		void combox::editable(bool eb)
		{
			get_drawer_trigger().get_drawer_impl().editable(eb);
		}

		bool combox::editable() const
		{
			return get_drawer_trigger().get_drawer_impl().editable();
		}

		void combox::set_accept(std::function<bool(nana::char_t)> pred)
		{
			internal_scope_guard lock;
			get_drawer_trigger().set_accept(std::move(pred));
		}

		combox& combox::push_back(const nana::string& text)
		{
			get_drawer_trigger().get_drawer_impl().insert(text);
			return *this;
		}

		std::size_t combox::the_number_of_options() const
		{
			return get_drawer_trigger().get_drawer_impl().the_number_of_options();
		}

		std::size_t combox::option() const
		{
			return get_drawer_trigger().get_drawer_impl().option();
		}

		void combox::option(std::size_t i)
		{
			get_drawer_trigger().get_drawer_impl().option(i, false);
			API::update_window(handle());
		}

		nana::string combox::text(std::size_t i) const
		{
			return get_drawer_trigger().get_drawer_impl().at(i).item_text;
		}

		void combox::erase(std::size_t pos)
		{
			get_drawer_trigger().get_drawer_impl().erase(pos);
		}

		void combox::renderer(item_renderer* ir)
		{
			get_drawer_trigger().get_drawer_impl().renderer(ir);
		}

		void combox::image(std::size_t i, const nana::paint::image& img)
		{
			if(empty()) return;

			auto & impl = get_drawer_trigger().get_drawer_impl();
			impl.image(i, img);
			if(i == impl.option())
				API::refresh_window(*this);
		}

		nana::paint::image combox::image(std::size_t pos) const
		{
			return get_drawer_trigger().get_drawer_impl().at(pos).item_image;
		}

		void combox::image_pixels(unsigned px)
		{
			if(get_drawer_trigger().get_drawer_impl().image_pixels(px))
				API::refresh_window(*this);
		}

		nana::string combox::_m_caption() const
		{
			internal_scope_guard isg;
			auto editor = get_drawer_trigger().get_drawer_impl().editor();
			return (editor ? editor->text() : nana::string());
		}

		void combox::_m_caption(nana::string&& str)
		{
			internal_scope_guard isg;
			get_drawer_trigger().get_drawer_impl().text(std::move(str));
			API::refresh_window(*this);
		}

		nana::any * combox::_m_anyobj(std::size_t pos, bool alloc_if_empty) const
		{
			return get_drawer_trigger().get_drawer_impl().anyobj(pos, alloc_if_empty);
		}

		auto combox::_m_at_key(std::shared_ptr<nana::detail::key_interface>&& p) -> item_proxy
		{
			auto & impl = get_drawer_trigger().get_drawer_impl();
			return item_proxy(&impl, impl.at_key(std::move(p)));
		}

		void combox::_m_erase(nana::detail::key_interface* p)
		{
			get_drawer_trigger().get_drawer_impl().erase(p);
		}
	//end class combox
}
