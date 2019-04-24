/*
 *	A Tooltip Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/tooltip.cpp
 */

#include <nana/gui/tooltip.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/timer.hpp>
#include <nana/gui/screen.hpp>
#include <memory>
#include <map>

namespace nana
{
	namespace drawerbase
	{
		namespace tooltip
		{
			class drawer
				: public drawer_trigger
			{
			private:
				void refresh(graph_reference graph)
				{
					graph.rectangle(false, colors::black);
					graph.rectangle(::nana::rectangle(graph.size()).pare_off(1), true, {0xf0, 0xf0, 0xf0});
				}
			};

			nana::point pos_by_screen(nana::point pos, const nana::size& sz, bool overlap_allowed)
			{
				auto scr_area = screen().from_point(pos).workarea();
				if (pos.x + static_cast<int>(sz.width) > scr_area.right())
					pos.x = scr_area.right() - static_cast<int>(sz.width);
				if (pos.x < scr_area.x)
					pos.x = scr_area.x;

				if (pos.y + static_cast<int>(sz.height) >= scr_area.bottom())
					pos.y = scr_area.bottom() - static_cast<int>(sz.height);
				else if (!overlap_allowed)
					pos.y += 20;	//Add some pixels to avoid overlapping between cursor and tip window.


				if (pos.y < scr_area.y)
					pos.y = scr_area.y;

				return pos;
			}

			class tip_form
				:	public widget_object<category::root_tag, drawer>,
					public tooltip_interface
			{
				typedef widget_object<category::root_tag, drawer> base_type;
			public:
				tip_form()
					:	base_type(nullptr, false, rectangle(), appear::bald<appear::floating>()),
						duration_(0)
				{
					API::take_active(this->handle(), false, nullptr);
					label_.create(*this);
					label_.format(true);
					label_.transparent(true);
				}
			private:
				//tooltip_interface implementation
				bool tooltip_empty() const override
				{
					return this->empty();
				}

				void tooltip_text(const std::string& text) override
				{
					label_.caption(text);
					auto text_s = label_.measure(screen().from_window(label_).workarea().width * 2 / 3);
					this->size(nana::size{ text_s.width + 10, text_s.height + 10 });
					label_.move(rectangle{ 5, 5, text_s.width, text_s.height });

					timer_.reset();
					if (duration_)
					{
						timer_.interval(std::chrono::milliseconds{ duration_ });
						timer_.elapse(std::bind(&tip_form::_m_tick_duration, this));
					}
					else
					{
						timer_.interval(std::chrono::milliseconds{ 500 });
						timer_.elapse(std::bind(&tip_form::_m_tick, this));
					}
					timer_.start();
				}

				virtual nana::size tooltip_size() const override
				{
					return this->size();
				}

				virtual void tooltip_move(const nana::point& scr_pos, bool ignore_pos) override
				{
					ignore_pos_ = ignore_pos;
					pos_ = scr_pos;
					if (duration_)
					{
						this->move(scr_pos.x, scr_pos.y);
						this->show();
					}
				}

				virtual void duration(std::size_t d) override
				{
					duration_ = d;
					timer_.reset();
				}
			private:
				void _m_tick()
				{
					nana::point pos;
					if (ignore_pos_)
					{
						pos = API::cursor_position();

						//The cursor must be stay here for half second.
						if (pos != pos_)
						{
							pos_ = pos;
							return;
						}
						
						pos = pos_by_screen(pos, size(), false);
					}
					else
						pos = pos_;

					timer_.stop();
					move(pos.x, pos.y);
					show();
				}

				void _m_tick_duration()
				{
					timer_.reset();
					this->close();
				}
			private:
				timer timer_;
				nana::label label_;
				nana::point pos_;
				bool		ignore_pos_;
				std::size_t	duration_;
			};//end class tip_form

			class controller
			{
				struct tip_value
				{
					std::string text;
					event_handle evt_msenter;
					event_handle evt_msleave;
					event_handle evt_msdown;
					event_handle evt_destroy;
				};

				typedef std::function<void(tooltip_interface*)> deleter_type;

				class tip_form_factory
					: public nana::tooltip::factory_if_type
				{
					tooltip_interface * create() override
					{
						return new tip_form;
					}

					void destroy(tooltip_interface* p) override
					{
						delete p;
					}
				};

			public:
				static std::shared_ptr<nana::tooltip::factory_if_type>& factory()
				{
					static std::shared_ptr<nana::tooltip::factory_if_type> fp;
					if (nullptr == fp)
						fp = std::make_shared<tip_form_factory>();

					return fp;
				}

				//external synchronization.
				static controller* instance(bool destroy = false)
				{
					static controller* ptr;

					if(destroy)
					{
						delete ptr;
						ptr = nullptr;
					}
					else if(nullptr == ptr)
					{
						ptr = new controller;
					}
					return ptr;
				}

				void set(window wd, const std::string& str)
				{
					if (str.empty())
						_m_untip(wd);
					else
						_m_get(wd).text = str;
				}

				void show(const std::string& text, const point* pos, std::size_t duration)
				{
					if (nullptr == window_ || window_->tooltip_empty())
					{
						auto fp = factory();

						window_ = std::unique_ptr<tooltip_interface, deleter_type>(fp->create(), [fp](tooltip_interface* ti)
						{
							fp->destroy(ti);
						});
					}

					window_->duration(duration);
					window_->tooltip_text(text);

					if (pos)
						window_->tooltip_move(pos_by_screen(*pos, window_->tooltip_size(), true), false);
					else
						window_->tooltip_move(API::cursor_position(), true);
				}

				void close()
				{
					window_.reset();

					//Destroy the tooltip controller when there are not tooltips.
					if (table_.empty())
						instance(true);
				}
			private:
				void _m_untip(window wd)
				{
					auto i = table_.find(wd);
					if(i != table_.end())
					{
						API::umake_event(i->second.evt_msdown);
						API::umake_event(i->second.evt_msenter);
						API::umake_event(i->second.evt_msleave);
						API::umake_event(i->second.evt_destroy);

						table_.erase(i);
					}

					if (table_.empty())
					{
						window_.reset();
						instance(true);
					}
				}
			private:
				tip_value& _m_get(window wd)
				{
					auto i = table_.find(wd);
					if (i != table_.end())
						return i->second;

					auto & events = API::events(wd);

					auto mouse_fn = [this](const arg_mouse& arg)
					{
						if (event_code::mouse_enter == arg.evt_code)
						{
							auto & value = _m_get(arg.window_handle);
							if (value.text.size())
								this->show(value.text, nullptr, 0);
						}
						else
							this->close();
					};

					auto & value = table_[wd];

					value.evt_msenter = events.mouse_enter.connect(mouse_fn);
					value.evt_msleave = events.mouse_leave.connect(mouse_fn);
					value.evt_msdown = events.mouse_down.connect(mouse_fn);

					value.evt_destroy = events.destroy.connect([this](const arg_destroy& arg){
						_m_untip(arg.window_handle);
					});

					return value;
				}
			private:
				std::unique_ptr<tooltip_interface, deleter_type> window_;
				std::map<window, tip_value> table_;
			};
		}//namespace tooltip
	}//namespace drawerbase

	//class tooltip
		typedef drawerbase::tooltip::controller ctrl;

		void tooltip::set(window wd, const std::string& text)
		{
			if(false == API::empty_window(wd))
			{
				internal_scope_guard lock;
				ctrl::instance()->set(wd, text);
			}
		}

		void tooltip::show(window wd, point pos, const std::string& text, std::size_t duration)
		{
			internal_scope_guard lock;
			API::calc_screen_point(wd, pos);
			ctrl::instance()->show(text, &pos, duration);
		}

		void tooltip::close()
		{
			internal_scope_guard lock;
			ctrl::instance()->close();
		}

		void tooltip::_m_hold_factory(factory_interface* p)
		{
			ctrl::factory().reset(p);
		}
	//end class tooltip
}//namespace nana
