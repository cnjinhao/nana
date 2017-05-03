/*
*	A Content View Implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2017 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/widgets/skeletons/content_view.hpp
*	@author: Jinhao
*/

#include "content_view.hpp"
#include <nana/gui/widgets/scroll.hpp>
#include <algorithm>

namespace nana {
	namespace widgets {
		namespace skeletons
		{
			struct content_view::implementation
			{
				content_view&	view;
				window const	window_handle;
				nana::rectangle	disp_area;
				nana::size		content_size;

				point skew_horz;
				point skew_vert;
				nana::size extra_px;

				bool	enable_update{ true };
				bool	drag_started{ false };
				point origin;

				nana::scroll<false>	horz;
				nana::scroll<true>	vert;

				timer tmr;

				events_type events;

				struct conf_provider
				{
					std::function<unsigned()> wheel_speed;
				}provider;

				implementation(content_view& v, window handle) :
					view(v),
					window_handle(handle)
				{
					API::events(handle).mouse_wheel.connect_unignorable([this](const arg_wheel& arg) {
						scroll_interface * scroll = nullptr;
						switch (arg.which)
						{
						case arg_wheel::wheel::vertical:
							scroll = &vert;
							break;
						case arg_wheel::wheel::horizontal:
							scroll = &horz;
							break;
						default:
							//Other button is not unsupported.
							return;
						}

						if (!API::empty_window(arg.window_handle))
						{
							auto align_px = (scroll->value() % scroll->step());
							if (align_px)
							{
								auto new_value = scroll->value() - align_px;
								if (!arg.upwards)
									new_value += scroll->step();

								scroll->value(new_value);
							}
							else
							{
								unsigned speed = 1;
								if (provider.wheel_speed)
								{
									speed = provider.wheel_speed();
									if (0 == speed)
										speed = 1;
								}
								scroll->make_step(!arg.upwards, speed);
							}
						}
					});

					auto mouse_evt = [this](const arg_mouse& arg)
					{
						if (event_code::mouse_down == arg.evt_code)
						{
							if (!arg.is_left_button())
								return;

							this->drag_started = this->view.view_area().is_hit(arg.pos);
						}
						else if (event_code::mouse_move == arg.evt_code)
						{
							if (this->drag_started && this->drive(arg.pos))
							{
								tmr.interval(16);
								tmr.start();
							}
						}
						else if (event_code::mouse_up == arg.evt_code)
						{
							this->drag_started = false;
							tmr.stop();
						}
					};

					API::events(handle).mouse_down.connect_unignorable(mouse_evt);
					API::events(handle).mouse_move.connect_unignorable(mouse_evt);
					API::events(handle).mouse_up.connect_unignorable(mouse_evt);

					tmr.elapse([this](const arg_elapse&)
					{
						auto curs = ::nana::API::cursor_position();
						::nana::API::calc_window_point(window_handle, curs);

						if (this->drive(curs))
						{
							if (events.hover_outside)
								events.hover_outside(curs);

							API::refresh_window(window_handle);
							view.sync(false);
						}
						else
							tmr.stop();
					});
				}

				bool drive(const point& cursor_pos)
				{
					auto const area = view.view_area();

					point skew;

					if (disp_area.x > cursor_pos.x)
						skew.x = cursor_pos.x - disp_area.x;
					else if (cursor_pos.x > disp_area.x + static_cast<int>(area.width))
						skew.x = cursor_pos.x - (disp_area.x + static_cast<int>(area.width));

					if (disp_area.y > cursor_pos.y)
						skew.y = cursor_pos.y - disp_area.y;
					else if (cursor_pos.y > disp_area.y + static_cast<int>(area.height))
						skew.y = cursor_pos.y - (disp_area.y + static_cast<int>(area.height));

					if (skew.x == 0 && skew.y == 0)
						return false;

					auto speed_horz = 0;
					if (skew.x)
						speed_horz = skew.x / (std::max)(1, static_cast<int>(horz.step())) + (skew.x < 0 ? -1 : 1);

					auto speed_vert = 0;
					if (skew.y)
						speed_vert = skew.y / (std::max)(1, static_cast<int>(vert.step())) + (skew.y < 0 ? -1 : 1);

					speed_horz = (std::min)(5, (std::max)(speed_horz, -5));
					speed_vert = (std::min)(5, (std::max)(speed_vert, -5));

					return view.move_origin({
						speed_horz, speed_vert
					});
				}

				void size_changed(bool try_update)
				{
					auto imd_area = view.view_area();

					//event hander for scrollbars
					auto event_fn = [this](const arg_scroll& arg)
					{
						if (arg.window_handle == this->vert.handle())
							origin.y = static_cast<int>(this->vert.value());
						else
							origin.x = static_cast<int>(this->horz.value());

						if (this->events.scrolled)
							this->events.scrolled();

						if (this->enable_update)
							API::refresh_window(this->window_handle);
					};

					this->enable_update = try_update;

					if (imd_area.width != disp_area.width)
					{
						if (vert.empty())
						{
							vert.create(window_handle);
							vert.events().value_changed.connect_unignorable(event_fn);
							API::take_active(vert, false, window_handle);
							this->enable_update = false;
						}
						
						vert.move({
							disp_area.x + static_cast<int>(imd_area.width) + skew_vert.x,
							disp_area.y + skew_vert.y,
							space(),
							imd_area.height + extra_px.height
						});

						vert.amount(content_size.height);
						vert.range(imd_area.height);
						vert.value(origin.y);
					}
					else
					{
						vert.close();
						origin.y = 0;
					}

					if (imd_area.height != disp_area.height)
					{
						if (horz.empty())
						{
							horz.create(window_handle);
							horz.events().value_changed.connect_unignorable(event_fn);
							API::take_active(horz, false, window_handle);
							this->enable_update = false;
						}

						horz.move({
							disp_area.x + skew_horz.x,
							disp_area.y + static_cast<int>(imd_area.height) + skew_horz.y,
							imd_area.width + extra_px.width,
							space()
						});

						horz.amount(content_size.width);
						horz.range(imd_area.width);
						horz.value(origin.x);
					}
					else
					{
						horz.close();
						origin.x = 0;
					}

					this->enable_update = true;
				}
			};

			content_view::content_view(window handle)
				: impl_{ new implementation{*this, handle} }
			{
			}

			content_view::~content_view()
			{
				delete impl_;
			}

			content_view::events_type& content_view::events()
			{
				return impl_->events;
			}

			void content_view::step(unsigned step_value, bool horz)
			{
				if (horz)
					impl_->horz.step(step_value);
				else
					impl_->vert.step(step_value);
			}

			bool content_view::scroll(bool forwards, bool horz)
			{
				unsigned speed = 1;
				if (impl_->provider.wheel_speed)
				{
					speed = impl_->provider.wheel_speed();
					if (0 == speed)
						speed = 1;
				}

				if (horz)
					return impl_->horz.make_step(forwards, speed);
				
				return impl_->vert.make_step(forwards, speed);
			}

			bool content_view::turn_page(bool forwards, bool horz)
			{
				if (horz)
					return impl_->horz.make_page_scroll(forwards);
				else
					return impl_->vert.make_page_scroll(forwards);
			}

			void content_view::disp_area(const rectangle& da, const point& skew_horz, const point& skew_vert, const size& extra_px, bool try_update)
			{
				if (impl_->disp_area != da)
				{
					impl_->disp_area = da;
					impl_->skew_horz = skew_horz;
					impl_->skew_vert = skew_vert;
					impl_->extra_px = extra_px;

					auto imd_area = this->view_area();
					if (static_cast<int>(impl_->content_size.width) - impl_->origin.x < static_cast<int>(imd_area.width))
						impl_->origin.x = (std::max)(0, static_cast<int>(impl_->content_size.width) - static_cast<int>(imd_area.width));

					if (static_cast<int>(impl_->content_size.height) - impl_->origin.y < static_cast<int>(imd_area.height))
						impl_->origin.y = (std::max)(0, static_cast<int>(impl_->content_size.height) - static_cast<int>(imd_area.height));

					impl_->size_changed(try_update);
				}
			}

			void content_view::content_size(const size& sz, bool try_update)
			{
				if (sz.height < impl_->content_size.height)
				{
					if (impl_->origin.y + impl_->disp_area.height > sz.height)
					{
						if (impl_->disp_area.height > sz.height)
							impl_->origin.y = 0;
						else
							impl_->origin.y = sz.height - impl_->disp_area.height;
					}
				}

				if (sz.width < impl_->content_size.width)
				{
					if (impl_->origin.x + impl_->disp_area.width > sz.width)
					{
						if (impl_->disp_area.width > sz.width)
							impl_->origin.x = 0;
						else
							impl_->origin.x = sz.width - impl_->disp_area.width;
					}
				}


				impl_->content_size = sz;

				impl_->size_changed(try_update);
			}

			const size& content_view::content_size() const
			{
				return impl_->content_size;
			}

			const point& content_view::origin() const
			{
				return impl_->origin;
			}

			rectangle content_view::corner() const
			{
				rectangle r;

				auto imd_area = this->view_area();

				r.x = impl_->disp_area.x + static_cast<int>(imd_area.width) + impl_->skew_vert.x;
				r.y = impl_->disp_area.y + static_cast<int>(imd_area.height) + impl_->skew_horz.y;


				unsigned extra_horz = (impl_->disp_area.width < impl_->content_size.width ? space() : 0);
				unsigned extra_vert = (impl_->disp_area.height < impl_->content_size.height + extra_horz ? space() : 0);

				if ((0 == extra_horz) && extra_vert)
					extra_horz = (impl_->disp_area.width < impl_->content_size.width + extra_vert ? space() : 0);

				r.width = extra_horz;
				r.height = extra_vert;

				return r;
			}

			void content_view::draw_corner(graph_reference graph)
			{
				auto r = corner();
				if(!r.empty())
					graph.rectangle(r, true, colors::button_face);
			}

			rectangle content_view::view_area() const
			{
				unsigned extra_horz = (impl_->disp_area.width < impl_->content_size.width ? space() : 0);
				unsigned extra_vert = (impl_->disp_area.height < impl_->content_size.height + extra_horz ? space() : 0);

				if ((0 == extra_horz) && extra_vert)
					extra_horz = (impl_->disp_area.width < impl_->content_size.width + extra_vert ? space() : 0);

				return rectangle{
					impl_->disp_area.position(),
					size{
					impl_->disp_area.width > extra_vert ? impl_->disp_area.width - extra_vert : 0,
					impl_->disp_area.height > extra_horz ? impl_->disp_area.height - extra_horz : 0
				}
				};
			}

			unsigned content_view::extra_space(bool horz) const
			{
				return ((horz ? impl_->horz.empty() : impl_->vert.empty()) ? 0 : space());
			}

			void content_view::change_position(int pos, bool aligned, bool horz)
			{
				if (aligned)
					pos -= (pos % static_cast<int>(horz ? impl_->horz.step() : impl_->vert.step()));

				auto imd_size = this->view_area();

				if (horz)
				{
					if (pos + imd_size.width > impl_->content_size.width)
						pos = static_cast<int>(impl_->content_size.width) - static_cast<int>(imd_size.width);

					if (pos < 0)	pos = 0;

					impl_->origin.x = pos;
				}
				else
				{
					if (pos + imd_size.height > impl_->content_size.height)
						pos = static_cast<int>(impl_->content_size.height) - static_cast<int>(imd_size.height);

					if (pos < 0)	pos = 0;

					impl_->origin.y = pos;
				}
			}

			bool content_view::move_origin(const point& skew)
			{
				auto imd_area = this->view_area();

				auto pre_origin = impl_->origin;

				impl_->origin.x += skew.x;
				if (impl_->origin.x + imd_area.width > impl_->content_size.width)
					impl_->origin.x = static_cast<int>(impl_->content_size.width) - static_cast<int>(imd_area.width);

				if (impl_->origin.x < 0)	impl_->origin.x = 0;


				impl_->origin.y += skew.y;
				if (impl_->origin.y + imd_area.height > impl_->content_size.height)
					impl_->origin.y = static_cast<int>(impl_->content_size.height) - static_cast<int>(imd_area.height);

				if (impl_->origin.y < 0)	impl_->origin.y = 0;

				return (pre_origin != impl_->origin);
			}

			void content_view::sync(bool try_update)
			{
				impl_->enable_update = try_update;
				impl_->horz.value(impl_->origin.x);
				impl_->vert.value(impl_->origin.y);
				impl_->enable_update = true;
			}

			void content_view::pursue(const point& cursor)
			{
				if (impl_->disp_area.is_hit(cursor))
					return;

				int delta = 0;
				if (cursor.x < impl_->disp_area.x)
					delta = cursor.x - impl_->disp_area.x;
				else if (cursor.x > impl_->disp_area.right())
					delta = cursor.x - impl_->disp_area.right();

				impl_->origin.x += delta;
				if (impl_->origin.x < 0)
					impl_->origin.x = 0;

				if (cursor.y < impl_->disp_area.y)
					delta = cursor.y - impl_->disp_area.y;
				else if (cursor.y > impl_->disp_area.bottom())
					delta = cursor.y - impl_->disp_area.bottom();

				impl_->origin.y += delta;
				if (impl_->origin.y < 0)
					impl_->origin.y = 0;

				bool changed = false;
				if (!impl_->horz.empty() && (static_cast<long long>(impl_->horz.value()) != impl_->origin.x))
				{
					impl_->horz.value(impl_->origin.x);
					changed = true;
				}

				if ((!impl_->vert.empty()) && (static_cast<long long>(impl_->vert.value()) != impl_->origin.y))
				{
					impl_->vert.value(impl_->origin.y);
					changed = true;
				}

				if (changed)
					API::refresh_window(impl_->window_handle);
			}

			void content_view::set_wheel_speed(std::function<unsigned()> fn)
			{
				impl_->provider.wheel_speed = std::move(fn);
			}
		}
	}
}
