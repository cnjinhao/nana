/*
 *	A platform API implementation
 *	Copyright(C) 2003-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/system/platform.cpp
 *	@description:
 *		this implements some API for platform-independent programming
 *	@contributors:
 *		Benjamin Navarro(pr#81)
 */
#include <nana/deploy.hpp>

#if defined(NANA_WINDOWS)
	#include <windows.h>
	#include "../detail/mswin/platform_spec.hpp"
#elif defined(NANA_POSIX)
	#include <time.h>
	#include <errno.h>
	#include <unistd.h>
	#include <sys/time.h>
	#include <sys/syscall.h>
	#include <pthread.h>
	#include <sys/stat.h>
	#include <spawn.h>
	#include <string.h>

static void posix_open_url(const char *url_utf8)
{
    extern char **environ;
    const char *home = getenv("HOME");
    std::string cheat(home);
    cheat += "/.mozilla";
    struct stat exists;

    // TODO: generalize this for chromium, opera, waterfox, etc.
    // Most desktop environments (KDE, Gnome, Lumina etc.) provide a way to set
    // your preferred browser - but there are more desktops than browsers.

    // Look for $HOME/.mozilla directory as strong evidence they use firefox.
    if ( stat(cheat.c_str(), &exists) == 0 && S_ISDIR(exists.st_mode))
    {
        const char *path = "";
        static const char *likely[2] = { "/usr/local/bin/firefox", "/usr/bin/firefox"};
        if ( stat(likely[0], &exists) == 0 && S_ISREG(exists.st_mode))
            path = likely[0];
        else if ( stat(likely[1], &exists) == 0 && S_ISREG(exists.st_mode) )
            path = likely[1];
        else return;

        pid_t pid = 0;
        static const char firefox[] = "firefox";
        char name[sizeof firefox]{};
        // argv does not like const-literals so make a copy.
        strcpy(name, firefox);
        char *argv[3] = {name, const_cast<char *>(url_utf8), nullptr};
        posix_spawn(&pid, path, NULL, NULL, argv, environ);
    }
}
#endif

namespace nana
{
namespace system
{
	//sleep
	//@brief:	Suspend current thread for a specified milliseconds.
	//			its precision is depended on hardware.
	void sleep(unsigned milliseconds)
	{
#if defined(NANA_WINDOWS)
		::Sleep(milliseconds);
#elif defined(NANA_POSIX)
		struct timespec timeOut, remains;
		timeOut.tv_sec = milliseconds / 1000;
		timeOut.tv_nsec = (milliseconds % 1000) * 1000000;
		while(-1 == ::nanosleep(&timeOut, &remains))
		{
			if(errno == EINTR)
				timeOut = remains;
			else
				break;
		}
#endif
	}

	//this_thread_id
	//@brief: get the identifier of calling thread.
	thread_t this_thread_id()
	{
#if defined(NANA_WINDOWS)
		return (thread_t)::GetCurrentThreadId();
#elif defined(NANA_LINUX)
		return (thread_t)::syscall(__NR_gettid);
#elif defined(NANA_MACOS)
		return (thread_t)::syscall(SYS_thread_selfid);
#elif defined(NANA_POSIX)
        return (thread_t)pthread_self();
#endif
	}

	unsigned long timestamp()
	{
#if defined(NANA_WINDOWS)
		return ::GetTickCount();
#elif defined(NANA_POSIX)
		struct timeval tv;
		::gettimeofday(&tv, 0);
		return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
	}

	bool get_async_mouse_state(int button)
	{
#if defined(NANA_WINDOWS)
		bool swap = (::GetSystemMetrics(SM_SWAPBUTTON) != 0);
		switch(button)
		{
		case 1: //Right
			button = swap ? VK_LBUTTON : VK_RBUTTON;
			break;
		case 2:
			button = VK_MBUTTON;
			break;
		default:
			button = swap ? VK_RBUTTON : VK_LBUTTON;
			break;
		}

		return (::GetAsyncKeyState(button) != 0);
#elif defined(NANA_POSIX)
		static_cast<void>(button);	//eliminate unused parameter compiler warning.
		return false;
#endif
	}

	//open an url through a default browser
	void open_url(const std::string& url_utf8)
	{
		if(url_utf8.empty())
			return;

#if defined(NANA_WINDOWS)
		if(::ShellExecute(0, L"open", nana::to_wstring(url_utf8).c_str(), 0, 0, SW_SHOWNORMAL) < reinterpret_cast<HINSTANCE>(32))
		{
			//Because ShellExecute can delegate execution to Shell extensions (data sources, context menu handlers,
			//verb implementations) that are activated using Component Object Model (COM), COM should be initialized
			//before ShellExecute is called. Some Shell extensions require the COM single-threaded apartment (STA) type.
			//In that case, COM should be initialized under WinXP.
			nana::detail::platform_spec::co_initializer co_init;
			::ShellExecute(0, L"open", nana::to_wstring(url_utf8).c_str(), 0, 0, SW_SHOWNORMAL);
		}
#elif defined(NANA_POSIX)
        posix_open_url(url_utf8.c_str());
#endif
    }
}//end namespace system
}//end namespace nana
