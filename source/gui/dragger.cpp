
#include <nana/gui/dragger.hpp>

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
		dragger_impl_t()
			: dragging_(false)
		{}

		~dragger_impl_t()
		{
			_m_clear_triggers();
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
			auto fn = std::bind(&dragger_impl_t::_m_trace, this, std::placeholders::_1);
			auto & events = API::events(wd);
			tg.press	= events.mouse_down.connect(fn);
			tg.over		= events.mouse_move.connect(fn);
			tg.release	= events.mouse_up.connect(fn);
			tg.destroy = events.destroy.connect([this](const arg_destroy& arg){
				_m_destroy(arg.window_handle);
			});

			triggers_.push_back(tg);
		}
	private:
		void _m_clear_triggers()
		{
			for(auto & t : triggers_)
			{
				API::umake_event(t.press);
				API::umake_event(t.over);
				API::umake_event(t.release);
				API::umake_event(t.destroy);
				API::capture_window(t.wd, false);
			}
			triggers_.clear();
		}

		void _m_destroy(::nana::window wd)
		{
			for(auto i = triggers_.begin(), end = triggers_.end(); i != end; ++i)
			{
				if(i->wd == wd)
				{
					triggers_.erase(i);
					API::capture_window(wd, false);
					return;
				}
			}
		}

		void _m_check_restrict_area(nana::point & pos, const nana::size & size, const nana::rectangle& restr_area)
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

		void _m_trace(const arg_mouse& arg)
		{
			switch(arg.evt_code)
			{
			case event_code::mouse_down:
				dragging_ = true;
				API::capture_window(arg.window_handle, true);
				origin_ = API::cursor_position();
				for(auto & t : targets_)
				{
					t.origin = API::window_position(t.wd);
					window owner = API::get_owner_window(t.wd);
					if(owner)
						API::calc_screen_point(owner, t.origin);
				}
				break;
			case event_code::mouse_move:
				if(dragging_ && arg.left_button)
				{
					auto pos = API::cursor_position();
					pos -= origin_;

					for(auto & t : targets_)
					{
						if(API::is_window_zoomed(t.wd, true) == false)
						{
							auto owner = API::get_owner_window(t.wd);
							auto wdps = t.origin;
							if (owner)
								API::calc_window_point(owner, wdps);

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

							API::move_window(t.wd, wdps.x, wdps.y);
						}
					}
				}
				break;
			case event_code::mouse_up:
				API::capture_window(arg.window_handle, false);
				dragging_ = false;
				break;
			default:
				break;
			}
		}

	private:
		bool dragging_;
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
