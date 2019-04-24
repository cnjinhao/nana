/*
 *	Implementation of Notifier
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/notifier.cpp
 *	@contributors:
 *		Jan
 *		Benjamin Navarro(pr#81)
 */
#include <nana/deploy.hpp>
#include <nana/gui/programming_interface.hpp>
#include <nana/gui/notifier.hpp>
#include <nana/gui/timer.hpp>

#include <unordered_map>
#include <unordered_set>

#if defined(STD_THREAD_NOT_SUPPORTED)
#include <nana/std_mutex.hpp>
#else
#include <mutex>
#endif

#include "../detail/platform_spec_selector.hpp"

#if defined(NANA_POSIX)
#include <nana/system/platform.hpp>
#include <iostream>
#endif

#include "../paint/image_accessor.hpp"

namespace nana
{
	using lock_guard = std::lock_guard<std::recursive_mutex>;

	struct notifier::implement
	{
		nana::timer	ani_timer;
		native_window_type	native_handle;
		window				handle;
		event_handle		evt_destroy;
		unsigned short id;
		detail::notifier_events events;
		bool icon_added = false;
		std::size_t	play_index;

		paint::image icon;
		::std::vector<paint::image> icons;

		void set_icon(const paint::image& ico)
		{
#if defined(NANA_WINDOWS)
			auto ico_handle = paint::image_accessor::icon(ico);
			if (ico_handle)
			{
				NOTIFYICONDATA icon_data;
				memset(&icon_data, 0, sizeof icon_data);
				icon_data.cbSize = sizeof icon_data;
				icon_data.hWnd = reinterpret_cast<HWND>(native_handle);
				icon_data.uID = id;
				icon_data.uFlags = NIF_MESSAGE | NIF_ICON;
				icon_data.uCallbackMessage = nana::detail::messages::tray;
				icon_data.hIcon = ico_handle;

				::Shell_NotifyIcon(icon_added ? NIM_MODIFY : NIM_ADD, &icon_data);
				icon_added = true;
			}
#else
			static_cast<void>(ico);
#endif
		}
	};

	arg_notifier::operator nana::arg_mouse() const
	{
		arg_mouse arg;
		arg.evt_code = evt_code;
		arg.ctrl = false;
		arg.shift = false;
		arg.left_button = left_button;
		arg.mid_button = mid_button;
		arg.right_button = right_button;
		arg.pos = pos;
		arg.window_handle = (notifier_ptr ? notifier_ptr->handle() : nullptr);
		return arg;
	}

	class notifications
	{
		struct notifier_data
		{
			::nana::notifier * notifier_ptr;
			detail::notifier_events* evt_ptr;
		};

		struct window_notifiers
		{
			std::unordered_map<unsigned short, notifier_data> idtable;
		};
	public:
		static notifications& instance()
		{
			static notifications obj;
			return obj;
		}

		unsigned short register_wd(::nana::notifier* ntf_ptr, native_window_type native_handle, detail::notifier_events* evt_ptr)
		{
			lock_guard lock(mutex_);
			auto i = wd_table_.find(native_handle);
			if (i == wd_table_.end())
			{
				auto & data = wd_table_[native_handle].idtable[0];
				data.notifier_ptr = ntf_ptr;
				data.evt_ptr = evt_ptr;
				return 0;
			}

			auto & idtable = i->second.idtable;
			auto id = static_cast<unsigned short>(idtable.size());

			const auto orig_id = id;
			while (idtable.count(id) != 0)
			{
				if (orig_id == ++id)
					throw std::runtime_error("Full");
			}

			auto & data = idtable[id];
			data.notifier_ptr = ntf_ptr;
			data.evt_ptr = evt_ptr;
			return id;
		}

		void cancel(native_window_type wd, unsigned short id)
		{
			lock_guard lock(mutex_);
			auto i_wd = wd_table_.find(wd);
			if (i_wd == wd_table_.end())
				return;

			auto & idtable = i_wd->second.idtable;
			auto i_id = idtable.find(id);
			if (i_id == idtable.end())
				return;

			idtable.erase(i_id);
			if (idtable.empty())
				wd_table_.erase(i_wd);
		}

		void emit(native_window_type wd, unsigned short id, const arg_notifier& arg_basic)
		{
			lock_guard lock(mutex_);
			auto i_wd = wd_table_.find(wd);
			if (i_wd == wd_table_.end())
				return;

			auto & idtable = i_wd->second.idtable;
			auto i_id = idtable.find(id);
			if (i_id == idtable.end())
				return;

			auto arg = arg_basic;

			arg.notifier_ptr = i_id->second.notifier_ptr;

			auto evt_ptr = i_id->second.evt_ptr;
			switch (arg.evt_code)
			{
			case event_code::mouse_down:
				evt_ptr->mouse_down.emit(arg, nullptr);
				break;
			case event_code::mouse_up:
				evt_ptr->mouse_up.emit(arg, nullptr);
				break;
			case event_code::mouse_leave:
				evt_ptr->mouse_leave.emit(arg, nullptr);
				break;
			case event_code::mouse_move:
				evt_ptr->mouse_move.emit(arg, nullptr);
				break;
			case event_code::dbl_click:
				evt_ptr->dbl_click.emit(arg, nullptr);
				break;
			default:
				break;
			}
		}
	private:
		mutable std::recursive_mutex mutex_;
		std::unordered_map<native_window_type, window_notifiers> wd_table_;
	};

#if defined(NANA_WINDOWS)
	void notifications_window_proc(HWND wd, WPARAM wparam, LPARAM lparam)
	{
		arg_notifier arg = {};
		switch (lparam)
		{
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
			arg.evt_code = event_code::dbl_click;
			arg.left_button = (lparam == WM_LBUTTONDBLCLK);
			arg.mid_button = (lparam == WM_MBUTTONDBLCLK);
			arg.right_button = (lparam == WM_RBUTTONDBLCLK);
			break;
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			arg.evt_code = event_code::mouse_down;
			arg.left_button = (lparam == WM_LBUTTONDOWN);
			arg.mid_button = (lparam == WM_MBUTTONDOWN);
			arg.right_button = (lparam == WM_RBUTTONDOWN);
			break;
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
			arg.evt_code = event_code::mouse_up;
			arg.left_button = (lparam == WM_LBUTTONUP);
			arg.mid_button = (lparam == WM_MBUTTONUP);
			arg.right_button = (lparam == WM_RBUTTONUP);
			break;
		case WM_MOUSEMOVE:
		case WM_MOUSELEAVE:
			arg.evt_code = (WM_MOUSEMOVE == lparam ? event_code::mouse_move : event_code::mouse_leave);
			arg.left_button = false;
			arg.mid_button = false;
			arg.right_button = false;
			break;
		default:
			return;
		}

		::POINT pos;
		::GetCursorPos(&pos);
		arg.pos.x = pos.x;
		arg.pos.y = pos.y;

		notifications::instance().emit(reinterpret_cast<native_window_type>(wd), static_cast<unsigned short>(wparam), arg);
	}
#endif

	//class notifier
	notifier::notifier(window wd)
		: impl_(new implement)
	{
		impl_->handle = wd;
		impl_->native_handle = API::root(wd);
		if (!impl_->native_handle)
			throw std::invalid_argument("Invalid window handle");

		impl_->ani_timer.elapse([this]
		{
#if defined(NANA_WINDOWS)
			if (impl_->play_index >= impl_->icons.size())
				impl_->play_index = 0;

			auto ico = impl_->icons[impl_->play_index++];
			impl_->set_icon(ico);
#else
			//eliminates warnings in clang
			static_cast<void>(this);
#endif
		});

		impl_->evt_destroy = API::events(wd).destroy.connect([this](const arg_destroy&)
		{
			close();
		});
		impl_->id = notifications::instance().register_wd(this, impl_->native_handle, &impl_->events);
	}

	notifier::~notifier()
	{
		close();
		delete impl_;
	}

	void notifier::close()
	{
		if (nullptr == impl_->native_handle)
			return;

#if defined(NANA_WINDOWS)
		NOTIFYICONDATA icon_data;
		memset(&icon_data, 0, sizeof icon_data);
		icon_data.cbSize = sizeof icon_data;
		icon_data.hWnd = reinterpret_cast<HWND>(impl_->native_handle);
		icon_data.uID = impl_->id;
		::Shell_NotifyIcon(NIM_DELETE, &icon_data);
#endif
		API::umake_event(impl_->evt_destroy);
		notifications::instance().cancel(impl_->native_handle, impl_->id);
		impl_->native_handle = nullptr;
	}

	void notifier::text(const std::string& str)
	{
#if defined(NANA_WINDOWS)
		NOTIFYICONDATAW icon_data;
		memset(&icon_data, 0, sizeof icon_data);
		icon_data.cbSize = sizeof icon_data;
		icon_data.hWnd = reinterpret_cast<HWND>(impl_->native_handle);
		icon_data.uID = impl_->id;
		icon_data.uFlags = NIF_MESSAGE | NIF_TIP;
		icon_data.uCallbackMessage = nana::detail::messages::tray;

		std::wcscpy(icon_data.szTip, to_wstring(str).c_str());

		::Shell_NotifyIcon(impl_->icon_added ? NIM_MODIFY : NIM_ADD, &icon_data);
		impl_->icon_added = true;
#else
		static_cast<void>(str); //to eliminate unused parameter compiler warning.
#endif
	}

	void notifier::icon(const std::string& icon_file)
	{
#if defined(NANA_WINDOWS)
		paint::image image_ico{ icon_file };
		auto icon_handle = paint::image_accessor::icon(image_ico);
		if (icon_handle)
		{
			impl_->ani_timer.stop();
			impl_->play_index = 0;
			impl_->set_icon(image_ico);
			impl_->icon = image_ico;
		}
#else
		static_cast<void>(icon_file);	//eliminate unused parameter warning
#endif
	}

	void notifier::insert_icon(const std::string& icon_file)
	{
#if defined(NANA_WINDOWS)
		paint::image image_ico{ icon_file };
		auto icon_handle = paint::image_accessor::icon(image_ico);
		if (icon_handle)
			impl_->icons.emplace_back(static_cast<paint::image&&>(image_ico));
#else
		static_cast<void>(icon_file);	//eliminate unused parameter warning
#endif
	}

	void notifier::period(std::chrono::milliseconds ms)
	{
#if defined(NANA_WINDOWS)
		if (ms.count() && impl_->icons.size())
		{
			auto frame_ms = (std::max)(ms.count() / static_cast<long long>(impl_->icons.size()), static_cast<long long>(16));

			impl_->ani_timer.interval(std::chrono::milliseconds{frame_ms});
			impl_->ani_timer.start();
		}
		else
			impl_->ani_timer.stop();
#else
		static_cast<void>(ms); //to eliminate unused parameter compiler warning.
#endif
	}

	detail::notifier_events& notifier::events()
	{
		return impl_->events;
	}

	window notifier::handle() const
	{
		return impl_->handle;
	}
	//end notifier
}
