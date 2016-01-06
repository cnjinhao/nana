/*
 *	A Tooltip Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
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
					:	base_type(nana::rectangle(), appear::bald<appear::floating>()),
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
						timer_.interval(static_cast<unsigned>(duration_));
						timer_.elapse(std::bind(&tip_form::_m_tick_duration, this));
					}
					else
					{
						timer_.interval(500);
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
				typedef std::pair<window, std::string> pair_t;

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
						_m_get(wd).second = str;
				}

				void show(const std::string& text)
				{
					if (nullptr == window_ || window_->tooltip_empty())
					{
						auto fp = factory();

						window_ = std::unique_ptr<tooltip_interface, deleter_type>(fp->create(), [fp](tooltip_interface* ti)
						{
							fp->destroy(ti);
						});
					}

					window_->duration(0);
					window_->tooltip_text(text);
					window_->tooltip_move(API::cursor_position(), true);
				}

				void show_duration(window wd, point pos, const std::string& text, std::size_t duration)
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

					pos = pos_by_screen(pos, window_->tooltip_size(), true);
					window_->tooltip_move(pos, false);
				}

				void close()
				{
					window_.reset();

					//Destroy the tooltip controller when there are not tooltips.
					if (cont_.empty())
						instance(true);
				}
			private:
				void _m_untip(window wd)
				{
					for (auto i = cont_.begin(); i != cont_.end(); ++i)
					{
						if (i->first == wd)
						{
							cont_.erase(i);

							if (cont_.empty())
							{
								window_.reset();
								instance(true);
							}
							return;
						}
					}
				}
			private:
				pair_t& _m_get(window wd)
				{
					for (auto & pr : cont_)
					{
						if (pr.first == wd)
							return pr;
					}

					auto & events = API::events(wd);
					events.mouse_enter.connect([this](const arg_mouse& arg){
						auto & pr = _m_get(arg.window_handle);
						if (pr.second.size())
							this->show(pr.second);
					});

					auto leave_fn = [this]{
						this->close();
					};
					events.mouse_leave.connect(leave_fn);
					events.mouse_down.connect(leave_fn);

					events.destroy.connect([this](const arg_destroy& arg){
						_m_untip(arg.window_handle);
					});

					cont_.emplace_back(wd, std::string());
					return cont_.back();
				}
			private:
				std::unique_ptr<tooltip_interface, deleter_type> window_;
				std::vector<pair_t> cont_;
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
			ctrl::instance()->show_duration(wd, pos, text, duration);
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
