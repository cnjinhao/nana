/*
 *	A Timer Implementation
 *  Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/timer.hpp
 *	@description:
 *		A timer can repeatedly call a piece of code. The duration between
 *	calls is specified in milliseconds. Timer is different from other graphics
 *	controls, it has no graphics interface.
 *	@contributors: Benjamin Navarro(pr#81)
 */
#include <nana/deploy.hpp>
#include <nana/gui/timer.hpp>
#include <map>
#include <memory>

#if defined(STD_THREAD_NOT_SUPPORTED)
    #include <nana/std_mutex.hpp>
#else
    #include <mutex>
#endif

#if defined(NANA_WINDOWS)
#include <windows.h>
#elif defined(NANA_POSIX)
#include "../detail/posix/platform_spec.hpp"
#include <nana/system/platform.hpp>
#endif

namespace nana
{
	namespace detail
	{
    	class timer_core;
    }

#if defined(NANA_WINDOWS)
	typedef UINT_PTR timer_identifier;
#else
	typedef const detail::timer_core* timer_identifier;
#endif

	class timer_driver
	{
		friend class detail::timer_core;

		timer_driver() = default;
	public:
		static timer_driver& instance()
		{
			static timer_driver obj;
			return obj;
		}

		template<typename Factory>
		detail::timer_core* create(unsigned ms, Factory && factory)
		{
#if defined(NANA_WINDOWS)
			auto tmid = ::SetTimer(nullptr, 0, ms, &timer_driver::_m_timer_proc);
#endif
			try
			{
#if defined(NANA_WINDOWS)
				auto p = factory(tmid);
#else
				auto p = factory();
				auto tmid = p;
				::nana::detail::platform_spec::instance().set_timer(tmid, ms, &timer_driver::_m_timer_proc);
#endif
				::nana::internal_scope_guard lock;
				timer_table_[tmid].reset(p);
				return p;
			}
			catch (...)
			{
			}
			return nullptr;
		}

		void destroy(timer_identifier handle)
		{
			::nana::internal_scope_guard lock;

			auto i = timer_table_.find(handle);
			if (i != timer_table_.end())
			{
#if defined(NANA_WINDOWS)
				::KillTimer(nullptr, handle);
#else
				::nana::detail::platform_spec::instance().kill_timer(handle);
#endif
				timer_table_.erase(i);
			}
		}
	private:
#if defined(NANA_WINDOWS)
		static void __stdcall _m_timer_proc(HWND hwnd, UINT uMsg, UINT_PTR id, DWORD dwTime);
#else
		static void _m_timer_proc(timer_identifier id);
#endif
	private:
		std::map<timer_identifier, std::unique_ptr<detail::timer_core>>	timer_table_;
	};

	class detail::timer_core
	{
	public:
#if defined(NANA_WINDOWS)
		timer_core(timer* sender, timer_identifier tmid, basic_event<arg_elapse>& evt_elapse):
			sender_(sender),
			timer_(tmid),
			evt_elapse_(evt_elapse)
		{}
#else
		timer_core(timer* sender, basic_event<arg_elapse>& evt_elapse):
			sender_(sender),
			timer_(this),
			evt_elapse_(evt_elapse)
		{}
#endif

		timer_identifier id() const
		{
			return timer_;
		}

		void interval(unsigned ms)
		{
#if defined(NANA_WINDOWS)
			::SetTimer(nullptr, timer_, ms, &timer_driver::_m_timer_proc);
#else
			::nana::detail::platform_spec::instance().set_timer(timer_, ms, &timer_driver::_m_timer_proc);
#endif
		}

		void emit()
		{
			arg_elapse arg;
			arg.sender = sender_;
			evt_elapse_.emit(arg, nullptr);
		}
	private:
		timer * const sender_;
		const timer_identifier timer_;
		nana::basic_event<arg_elapse> & evt_elapse_;
	}; //end class timer_core


#if defined(NANA_WINDOWS)
	void __stdcall timer_driver::_m_timer_proc(HWND /*hwnd*/, UINT /*uMsg*/, UINT_PTR handle, DWORD /*dwTime*/)
#else
	void timer_driver::_m_timer_proc(timer_identifier handle)
#endif
	{
		auto & time_tbl = instance().timer_table_;

		::nana::internal_scope_guard lock;

		auto i = time_tbl.find(handle);
		if (i == time_tbl.end())
			return;

		i->second->emit();
	}

	struct timer::implement
	{
		unsigned interval{ 1000 }; //1 second in default
		detail::timer_core * tm_core{ nullptr };
	};

	//class timer
		timer::timer()
			: impl_(new implement)
		{
		}
		
		timer::timer(std::chrono::milliseconds ms):
			timer()
		{
			this->interval(ms);
		}

		timer::~timer()
		{
			stop();
			delete impl_;
		}

		void timer::reset()
		{
			stop();
			elapse_.clear();
		}

		void timer::start()
		{
			if (impl_->tm_core)
				return;
#if defined(NANA_WINDOWS)
			impl_->tm_core = timer_driver::instance().create(impl_->interval, [this](timer_identifier id)
			{
				return new detail::timer_core(this, id, elapse_);
			});
#else
			impl_->tm_core = timer_driver::instance().create(impl_->interval, [this]
			{
				return new detail::timer_core(this, elapse_);
			});
#endif
		}

		bool timer::started() const
		{
			return (nullptr != impl_->tm_core);
		}

		void timer::stop()
		{
			if (nullptr == impl_->tm_core)
				return;

			auto tmid = impl_->tm_core->id();
			impl_->tm_core = nullptr;
			timer_driver::instance().destroy(tmid);
		}

		void timer::interval(std::chrono::milliseconds ms)
		{
			if (ms.count() != static_cast<long long>(impl_->interval))
			{
				impl_->interval = static_cast<unsigned>(ms.count());
				if (impl_->tm_core)
					impl_->tm_core->interval(impl_->interval);
			}
		}

		unsigned timer::_m_interval() const
		{
			return impl_->interval;
		}
	//end class timer
}//end namespace nana
