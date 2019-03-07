/*
*	A Dragger Implementation
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/dragger.cpp
*/

#include <nana/gui/dragger.hpp>
#include <nana/gui/programming_interface.hpp>

namespace nana
{
	class dragger::dragger_impl_t
	{
		struct drag_target_t
		{
			window wd;
			rectangle restrict_area;
			arrange	move_direction;
			point		origin;

			drag_target_t(window w, const rectangle& r, arrange m)
				: wd(w), restrict_area(r), move_direction(m)
			{}
		};

		struct trigger_t
		{
			window wd;
			event_handle press;
			event_handle over;
			event_handle release;
			event_handle destroy;
		};
	public:
		~dragger_impl_t()
		{
			//Clear triggers
			for (auto & t : triggers_)
			{
				API::umake_event(t.press);
				API::umake_event(t.over);
				API::umake_event(t.release);
				API::umake_event(t.destroy);
				API::release_capture(t.wd);
			}
		}

		void drag_target(window wd, const rectangle& restrict_area, arrange arg)
		{
			for (auto & td : targets_)
			{
				if (td.wd == wd)
				{
					td.restrict_area = restrict_area;
					td.move_direction = arg;
					return;
				}
			}
			targets_.emplace_back(wd, restrict_area, arg);
		}

		void remove_target(window wd)
		{
			for (auto i = targets_.begin(); i != targets_.end(); ++i)
			{
				if (i->wd == wd)
				{
					targets_.erase(i);
					return;
				}
			}
		}

		void trigger(window wd)
		{
			trigger_t tg;
			tg.wd = wd;
			auto fn = [this](const arg_mouse& arg)
			{
				switch (arg.evt_code)
				{
				case event_code::mouse_down:
					dragging_ = true;
					API::set_capture(arg.window_handle, true);

					origin_ = API::cursor_position();
					for (auto & t : targets_)
					{
						t.origin = API::window_position(t.wd);
						API::calc_screen_point(API::get_owner_window(t.wd), t.origin);
					}
					break;
				case event_code::mouse_move:
					if (dragging_ && arg.left_button)
					{
						auto pos = API::cursor_position();
						pos -= origin_;

						for (auto & t : targets_)
						{
							if (API::is_window_zoomed(t.wd, true) == false)
							{
								auto wdps = t.origin;
								API::calc_window_point(API::get_owner_window(t.wd), wdps);

								switch (t.move_direction)
								{
								case nana::arrange::horizontal:
									wdps.x += pos.x;
									break;
								case nana::arrange::vertical:
									wdps.y += pos.y;
									break;
								default:
									wdps += pos;
								}

								if (!t.restrict_area.empty())
									_m_check_restrict_area(wdps, API::window_size(t.wd), t.restrict_area);

								API::move_window(t.wd, wdps);
							}
						}
					}
					break;
				case event_code::mouse_up:
					API::release_capture(arg.window_handle);

					dragging_ = false;
					break;
				default:
					break;
				}
			};
			auto & events = API::events(wd);
			tg.press	= events.mouse_down.connect(fn);
			tg.over		= events.mouse_move.connect(fn);
			tg.release	= events.mouse_up.connect(fn);
			tg.destroy = events.destroy.connect([this](const arg_destroy& arg)
			{
				for (auto i = triggers_.begin(), end = triggers_.end(); i != end; ++i)
				{
					if (i->wd == arg.window_handle)
					{
						triggers_.erase(i);
						API::release_capture(arg.window_handle);
						return;
					}
				}
			});

			triggers_.emplace_back(tg);
		}
	private:
		static void _m_check_restrict_area(nana::point & pos, const nana::size & size, const nana::rectangle& restr_area)
		{
			if ((pos.x > 0) && (static_cast<int>(size.width) + pos.x > restr_area.right()))
				pos.x = restr_area.right() - static_cast<int>(size.width);

			if (pos.x < restr_area.x)
				pos.x = restr_area.x;

			if ((pos.y > 0) && (static_cast<int>(size.height) + pos.y > restr_area.bottom()))
				pos.y = restr_area.bottom() - static_cast<int>(size.height);

			if (pos.y < restr_area.y)
				pos.y = restr_area.y;
		}
	private:
		bool dragging_{ false };
		nana::point origin_;
		std::vector<drag_target_t> targets_;
		std::vector<trigger_t> triggers_;
	};

	//class dragger
		dragger::dragger()
			: impl_(new dragger_impl_t)
		{
		}

		dragger::~dragger()
		{
			delete impl_;
		}

		dragger::dragger(dragger&& other)
			: impl_(other.impl_)
		{
			other.impl_ = nullptr;
		}

		dragger& dragger::operator=(dragger&& other)
		{
			if (this != &other)
			{
				delete impl_;
				impl_ = other.impl_;
				other.impl_ = nullptr;
			}
			return *this;
		}

		void dragger::target(window wd)
		{
			impl_->drag_target(wd, rectangle(), nana::arrange::horizontal_vertical);
		}

		void dragger::target(window wd, const rectangle& restrict_area, nana::arrange arg)
		{
			impl_->drag_target(wd, restrict_area, arg);
		}

		void dragger::remove_target(window wd)
		{
			impl_->remove_target(wd);
		}

		void dragger::trigger(window tg)
		{
			impl_->trigger(tg);
		}
	//end class dragger
}//end namespace nana
