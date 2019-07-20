/**
*	Definition of General Events
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/general_events.hpp
*/
#ifndef NANA_DETAIL_GENERAL_EVENTS_HPP
#define NANA_DETAIL_GENERAL_EVENTS_HPP

#include <nana/push_ignore_diagnostic>

#include <nana/gui/basis.hpp>
#include "event_code.hpp"
#include "internal_scope_guard.hpp"
#include "../../filesystem/filesystem.hpp"
#include <type_traits>
#include <functional>
#include <vector>

namespace nana
{
	namespace API
	{
		bool is_window(window);			///< Determines whether a window is existing, equal to !empty_window.
	}

	namespace detail
	{
		void events_operation_register(event_handle);

		class event_interface
		{
		public:
			virtual ~event_interface() = default;
			virtual void remove(event_handle) = 0;
		};

		class event_docker_interface
		{
		public:
			virtual ~event_docker_interface() = default;
			virtual event_interface*	get_event() const = 0;
		};


		struct docker_base
			: public event_docker_interface
		{
			event_interface * const event_ptr;
			bool flag_deleted;
			const bool unignorable;

			docker_base(event_interface*, bool unignorable_flag);
			detail::event_interface * get_event() const override;
		};

		class event_base
			: public detail::event_interface
		{
		public:
			~event_base();

			std::size_t length() const;
			void clear() noexcept;

			void remove(event_handle evt) override;
		protected:
			//class emit_counter is a RAII helper for emitting count
			//It is used for avoiding a try{}catch block which is required for some finial works when
			//event handlers throw exceptions. Precondition event_base.dockers_ != nullptr.
			class emit_counter
			{
			public:
				emit_counter(event_base*);
				~emit_counter();
			private:
				event_base * const evt_;
			};
			
			event_handle _m_emplace(detail::event_docker_interface*, bool in_front);
		protected:
			unsigned emitting_count_{ 0 };
			bool deleted_flags_{ false };
			std::vector<detail::event_docker_interface*> * dockers_{ nullptr };
		};
	}//end namespace detail

    /// base class for all event argument types
	class event_arg
	{
	public:
		virtual ~event_arg() = default;

        /// ignorable handlers behind the current one in a chain of event handlers will not get called.
		void stop_propagation() const;
		bool propagation_stopped() const;
	private:
		mutable bool stop_propagation_{ false };
	};

	struct general_events;

    /** @brief the type of the members of general_events. 
	*  
	*   It connect the functions to be called as response to the event and manages that chain of responses
	*   It is a functor, that get called to connect a "normal" response function, with normal "priority".
    *   If a response function need another priority (unignorable or called first) it will need to be connected with 
    *   the specific connect function not with the operator()	
	*   It also permit to "emit" that event, calling all the active responders.
	*/
	template<typename Arg>
	class basic_event : public detail::event_base
	{
	public:
		using arg_reference = const typename std::remove_reference<Arg>::type &;
	private:
		struct docker
			: public detail::docker_base
		{
			/// the callback/response function taking the typed argument
			std::function<void(arg_reference)> invoke;

			docker(basic_event * evt, std::function<void(arg_reference)> && ivk, bool unignorable_flag)
				: docker_base(evt, unignorable_flag), invoke(std::move(ivk))
			{}

			docker(basic_event * evt, const std::function<void(arg_reference)> & ivk, bool unignorable_flag)
				: docker_base(evt, unignorable_flag), invoke(ivk)
			{}
		};
	public:
		/// Creates an event handler at the beginning of event chain
		template<typename Function>
		event_handle connect_front(Function && fn)
		{
#ifdef __cpp_if_constexpr
			if constexpr(std::is_invocable_v<Function, arg_reference>)
			{
				return _m_emplace(new docker{ this, fn, false }, true);
			}
			else if constexpr(std::is_invocable_v<Function>)
			{
				return _m_emplace(new docker{ this, [fn](arg_reference) {
					fn();
				}, false }, true);
			}
#else
			using prototype = typename std::remove_reference<Function>::type;
			return _m_emplace(new docker(this, factory<prototype, std::is_bind_expression<prototype>::value>::build(std::forward<Function>(fn)), false), true);
#endif
		}

#ifndef __cpp_if_constexpr
		/// It will not get called if stop_propagation() was called.
		event_handle connect(void (*fn)(arg_reference))
		{
			return connect([fn](arg_reference arg){
				fn(arg);
			});
		}
#endif

		/// It will not get called if stop_propagation() was called, because it is set at the end of the chain..
		template<typename Function>
		event_handle connect(Function && fn)
		{
#ifdef __cpp_if_constexpr
			if constexpr(std::is_invocable_v<Function, arg_reference>)
			{
				return _m_emplace(new docker{ this, fn, false }, false);
			}
			else if constexpr(std::is_invocable_v<Function>)
			{
				return _m_emplace(new docker{ this, [fn](arg_reference) mutable{
					fn();
				}, false }, false);
			}
#else
			using prototype = typename std::remove_reference<Function>::type;
			return _m_emplace(new docker(this, factory<prototype, std::is_bind_expression<prototype>::value>::build(std::forward<Function>(fn)), false), false);
#endif
		}

		/// It will not get called if stop_propagation() was called.
        template<typename Function>
		event_handle operator()(Function&& fn)
		{
			return connect(std::forward<Function>(fn));
		}

		/// It will get called because it is unignorable.
        template<typename Function>
		event_handle connect_unignorable(Function && fn, bool in_front = false)
		{
#ifdef __cpp_if_constexpr
			if constexpr(std::is_invocable_v<Function, arg_reference>)
			{
				return _m_emplace(new docker{ this, fn, true }, in_front);
			}
			else if constexpr(std::is_invocable_v<Function>)
			{
				return _m_emplace(new docker{ this, [fn](arg_reference) mutable{
					fn();
				}, true }, in_front);
			}
#else
			using prototype = typename std::remove_reference<Function>::type;
			return _m_emplace(new docker(this, factory<prototype, std::is_bind_expression<prototype>::value>::build(std::forward<Function>(fn)), true), in_front);
#endif
		}

		void emit(arg_reference& arg, window window_handle)
		{
			internal_scope_guard lock;
			if (nullptr == dockers_)
				return;

			emit_counter ec(this);

			//The dockers may resize when a new event handler is created by a calling handler.
			//Traverses with position can avaid crash error which caused by a iterator which becomes invalid.
			for (std::size_t i = 0; i < dockers_->size(); ++i)
			{
				auto d = static_cast<docker*>(dockers_->data()[i]);
				if (d->flag_deleted || (arg.propagation_stopped() && !d->unignorable))
					continue;

				d->invoke(arg);

				if (window_handle && (!::nana::API::is_window(window_handle)))
					break;
			}
		}
	private:

#ifndef __cpp_if_constexpr
		template<typename Fn, bool IsBind>
		struct factory
		{
			static std::function<void(arg_reference)> build(Fn && fn)
			{
				return std::move(fn);
			}

			static std::function<void(arg_reference)> build(const Fn & fn)
			{
				return fn;
			}
		};

		template<typename Fn>
		struct factory<Fn, false>
		{
			typedef typename std::remove_reference<arg_reference>::type arg_type;
			typedef typename std::remove_reference<Fn>::type fn_type;

			template<typename Tfn>
			static std::function<void(arg_reference)> build(Tfn && fn)
			{
				typedef typename std::remove_reference<Tfn>::type type;
				return build_second(std::forward<Tfn>(fn), &type::operator());
			}

			template<typename Tfn, typename Ret>
			static std::function<void(arg_reference)> build_second(Tfn&& fn, Ret(fn_type::*)())
			{
				return [fn](arg_reference) mutable
				{
					fn();
				};
			}

			template<typename Tfn, typename Ret>
			static std::function<void(arg_reference)> build_second(Tfn&& fn, Ret(fn_type::*)()const)
			{
				return [fn](arg_reference) mutable
				{
					fn();
				};
			}

			template<typename Tfn, typename Ret>
			static std::function<void(arg_reference)> build_second(Tfn&& fn, Ret(fn_type::*)(arg_reference))
			{
				return std::forward<Tfn>(fn);
			}

			template<typename Tfn, typename Ret>
			static std::function<void(arg_reference)> build_second(Tfn&& fn, Ret(fn_type::*)(arg_reference)const)
			{
				return std::forward<Tfn>(fn);
			}

			template<typename Tfn, typename Ret, typename Arg2>
			static std::function<void(arg_reference)> build_second(Tfn&& fn, Ret(fn_type::*)(Arg2))
			{
				static_assert(std::is_convertible<arg_type, Arg2>::value, "The parameter type is not allowed, please check the function parameter type where you connected the event function.");
				return[fn](arg_reference arg) mutable
				{
					fn(arg);
				};
			}
			
			template<typename Tfn, typename Ret, typename Arg2>
			static std::function<void(arg_reference)> build_second(Tfn&& fn, Ret(fn_type::*)(Arg2)const)
			{
				static_assert(std::is_convertible<arg_type, Arg2>::value, "The parameter type is not allowed, please check the function parameter type where you connected the event function.");
				return [fn](arg_reference arg) mutable
				{
					fn(arg);
				};
			}
		};
		
		template<typename Ret, typename Arg2>
		struct factory < std::function<Ret(Arg2)>, false>
		{
			typedef typename std::remove_reference<arg_reference>::type arg_type;
			static_assert(std::is_convertible<arg_type, Arg2>::value, "The parameter type is not allowed, please check the function parameter type where you connected the event function.");

			static std::function<void(arg_reference)> build(const std::function<Ret(Arg2)>& fn)
			{
				return [fn](arg_reference arg) mutable{
					fn(arg);
				};
			}

			static std::function<void(arg_reference)> build_second(std::function<void(arg_reference)> && fn)
			{
				return std::move(fn);
			}
		};

		template<typename Ret>
		struct factory < std::function<Ret()>, false>
		{
			static std::function<void(arg_reference)> build(const std::function<Ret()>& fn)
			{
				return[fn](arg_reference) mutable{
					fn();
				};
			}
		};

		template<typename Ret>
		struct factory < Ret(*)(), false>
		{
			static std::function<void(arg_reference)> build(Ret(*fn)())
			{
				return[fn](arg_reference) mutable{
					fn();
				};
			}
		};

		template<typename Ret, typename Arg2>
		struct factory < Ret(*)(Arg2), false>
		{
			typedef typename std::remove_reference<arg_reference>::type arg_type;
			static_assert(std::is_convertible<arg_type, Arg2>::value, "The parameter type is not allowed, please check the function parameter type where you connected the event function.");

			static std::function<void(arg_reference)> build(Ret(*fn)(Arg2))
			{
				return[fn](arg_reference arg) mutable {
					fn(arg);
				};
			}
		};

		template<typename Ret>
		struct factory < Ret(), false>
		{
			static std::function<void(arg_reference)> build(Ret(*fn)())
			{
				return[fn](arg_reference){
					fn();
				};
			}
		};

		template<typename Ret, typename Arg2>
		struct factory < Ret(Arg2), false>
		{
			typedef typename std::remove_reference<arg_reference>::type arg_type;
			static_assert(std::is_convertible<arg_type, Arg2>::value, "The parameter type is not allowed, please check the function parameter type where you connected the event function.");

			static std::function<void(arg_reference)> build(Ret(*fn)(Arg2))
			{
				return[fn](arg_reference arg){
					fn(arg);
				};
			}
		};
#endif
	};
 
	struct arg_mouse
		: public event_arg
	{
		event_code evt_code; ///< what kind of mouse event?
		::nana::window window_handle;  ///< A handle to the event window
		::nana::point pos;   ///< cursor position in the event window
		::nana::mouse button;	///< indicates a button which triggers the event

		bool left_button;	///< true if mouse left button is pressed
		bool mid_button;	///< true if mouse middle button is pressed
		bool right_button;	///< true if mouse right button is pressed
		bool alt;			///< true if keyboard alt is pressed
		bool shift;			///< true if keyboard Shift is pressed
		bool ctrl;			///< true if keyboard Ctrl is pressed

		/// Checks if left button is operated,
		bool is_left_button() const
		{
			return (event_code::mouse_move == evt_code ? left_button : (mouse::left_button == button));
		}
	};

    /// \brief in arg_wheel event_code is event_code::mouse_wheel 
	
    /// The type arg_wheel is derived from arg_mouse, a handler 
    /// with prototype void(const arg_mouse&) can be set for mouse_wheel.
	struct arg_wheel : public arg_mouse
	{
		enum class wheel{
			vertical,
			horizontal
		};

		wheel	which;		///< which wheel is rotated
		bool	upwards;	///< true if the wheel is rotated to the top/left, depends on which and false otherwise
		unsigned distance;	///< expressed in multiples or divisions of 120
	};

	struct arg_dropfiles : public event_arg  
	{
		::nana::window	window_handle;				///<  A handle to the event window
		::nana::point	pos;						///<  cursor position in the event window
		std::vector<std::filesystem::path>	files;	///<  external filenames
	};

	struct arg_expose : public event_arg
	{
		::nana::window window_handle;	///< A handle to the event window
		bool exposed;	                ///< the window is visible?
	};

	struct arg_focus : public event_arg
	{
		/// A constant to indicate how keyboard focus emitted.
		enum class reason
		{
			general,	///< the focus is received by OS native window manager.
			tabstop,	///< the focus is received by pressing tab.
			mouse_press ///< the focus is received by pressing a mouse button.
		};

		::nana::window window_handle;			///< A handle to the event window
		::nana::native_window_type receiver;	///< it is a native window handle, and specified which window receives focus
		bool	getting;						///< the window received focus?
		reason	focus_reason;					///< determines how the widget receives keyboard focus, it is ignored when 'getting' is equal to false
	};

	struct arg_keyboard : public event_arg
	{
		event_code evt_code;	    ///< it is event_code::key_press in current event
		::nana::window window_handle;	///< A handle to the event window
		mutable wchar_t key;	///< the key corresponding to the key pressed
		mutable bool ignore;	    ///< this member is only available for key_char event, set 'true' to ignore the input.
		bool alt;					///< it is set to indicate the modifier key Alt just prior to the event.
		bool ctrl;	                ///< it is set to indicate the modifier key Ctrl just prior to the event.
		bool shift;	                ///< it is set to indicate the modifier key Shift just prior to the event.
	};

	struct arg_move : public event_arg
	{
		::nana::window window_handle;	///< A handle to the event window
		int x;	                        ///< 
		int y;	                        ///< 
	};

	struct arg_resized : public event_arg
	{
		::nana::window window_handle;	///< A handle to the event window
		unsigned width;	                ///< new width in pixels.
		unsigned height;	            ///< new height in pixels.
	};

	struct arg_resizing : public event_arg
	{
		::nana::window window_handle;	///< A handle to the event window
		window_border border;	        ///< the window is being resized by moving border
		mutable unsigned width;	        ///< new width in pixels. If it is modified, the window's width will be the modified value
		mutable unsigned height;	    ///< new height in pixels. If it is modified, the window's height will be the modified value
	};

	struct arg_unload : public event_arg
	{
		::nana::window window_handle;	///< A handle to the event window
		mutable bool cancel;	        ///< 
	};

	struct arg_destroy : public event_arg
	{
		::nana::window window_handle;	///< A handle to the event window
	};
    /// a higher level event argument than just mouse down
	struct arg_click : public event_arg
	{
		::nana::window window_handle;	///< A handle to the event window
		const arg_mouse* mouse_args{};	///< If it is not null, it refers to the mouse arguments for click event emitted by mouse, nullptr otherwise.
	};

    /// provides some fundamental events that every widget owns.
	struct general_events
	{
		virtual ~general_events() = default;
		basic_event<arg_mouse> mouse_enter;	///< the cursor enters the window
		basic_event<arg_mouse> mouse_move;	///< the cursor moves on the window
		basic_event<arg_mouse> mouse_leave;	///< the cursor leaves the window
		basic_event<arg_mouse> mouse_down;	///< the user presses the mouse button
		basic_event<arg_mouse> mouse_up;	///< the user presses the mouse button
		basic_event<arg_click> click;		///< the window is clicked, but occurs after mouse_down and before mouse_up
		basic_event<arg_mouse> dbl_click;	///< the window is double clicked
		basic_event<arg_wheel> mouse_wheel;	///< the mouse wheel rotates while the window has focus
		basic_event<arg_dropfiles>	mouse_dropfiles; ///< the mouse drops some external data while the window enable accepting files
		basic_event<arg_expose>	expose;		///< the visibility changes
		basic_event<arg_focus>	focus;		///< the window receives or loses keyboard focus
		basic_event<arg_keyboard>	key_press;   ///< a key is pressed while the window has focus. event code is event_code::key_press
		basic_event<arg_keyboard>	key_release; ///< a key is released while the window has focus. event code is event_code::key_release
		basic_event<arg_keyboard>	key_char;	///< a character, whitespace or backspace is pressed. event code is event_code::key_char
		basic_event<arg_keyboard>	shortkey;	///< a defined short key is pressed. event code is event_code::shortkey

		basic_event<arg_move>		move;		///< the window changes position
		basic_event<arg_resizing>	resizing;	///< the window is changing its size
		basic_event<arg_resized>	resized;	///< the window is changing its size

		basic_event<arg_destroy>	destroy;	///< the window is destroyed, but occurs when all children have been destroyed
	};

	namespace detail
	{
		struct events_root_extension
			: public general_events
		{
			basic_event<arg_unload>	unload;
		};
	}//end namespace detail
}//end namespace nana

#include <nana/pop_ignore_diagnostic>

#endif
