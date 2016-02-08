/**
*	Definition of General Events
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/detail/general_events.hpp
*/
#ifndef NANA_DETAIL_GENERAL_EVENTS_HPP
#define NANA_DETAIL_GENERAL_EVENTS_HPP

#include <nana/gui/basis.hpp>
#include "event_code.hpp"
#include "internal_scope_guard.hpp"
#include <type_traits>
#include <functional>
#include <memory>
#include <vector>

namespace nana
{
	namespace detail
	{
		class event_interface
		{
		public:
			virtual ~event_interface() = default;
			virtual void remove(event_handle) = 0;
		};

		class docker_interface
		{
		public:
			virtual ~docker_interface() = default;
			virtual event_interface*	get_event() const = 0;
		};

		void events_operation_register(event_handle);
		void events_operation_cancel(event_handle);
	}//end namespace detail

    /// base clase for all event argument types
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

    /// the type of the members of general_events 
	template<typename Arg>
	class basic_event : public detail::event_interface
	{
	public:
		using arg_reference = const typename std::remove_reference<Arg>::type &;
	private:
		struct docker
			: public detail::docker_interface
		{
			basic_event * const event_ptr;
			std::function<void(arg_reference)> invoke;

			bool flag_deleted{ false };
			bool unignorable{false};

			docker(basic_event * s, std::function<void(arg_reference)> && ivk, bool unignorable_flag)
				: event_ptr(s), invoke(std::move(ivk)), unignorable(unignorable_flag)
			{}

			docker(basic_event * s, const std::function<void(arg_reference)> & ivk, bool unignorable_flag)
				: event_ptr(s), invoke(ivk), unignorable(unignorable_flag)
			{}

			~docker()
			{
				detail::events_operation_cancel(reinterpret_cast<event_handle>(this));
			}

			detail::event_interface * get_event() const override
			{
				return event_ptr;
			}
		};

		//class emit_counter is a RAII helper for emitting count
		//It is used for avoiding a try{}catch block which is required for some finial works when
		//event handlers throw exceptions.
		class emit_counter
		{
		public:
			emit_counter(basic_event* evt)
				: evt_{evt}
			{
				++evt->emitting_count_;
			}

			~emit_counter()
			{
				if ((0 == --evt_->emitting_count_) && evt_->deleted_flags_)
				{
					evt_->deleted_flags_ = false;
					for (auto i = evt_->dockers_->begin(); i != evt_->dockers_->end();)
					{
						if (static_cast<docker*>(i->get())->flag_deleted)
							i = evt_->dockers_->erase(i);
						else
							++i;
					}
				}
			}
		private:
			basic_event * const evt_;
		};
	public:
        /// It will get called firstly, because it is set at the beginning of the chain.
		template<typename Function>
		event_handle connect_front(Function && fn)
		{
			internal_scope_guard lock;
			if (nullptr == dockers_)
				dockers_.reset(new std::vector<std::unique_ptr<detail::docker_interface>>);

			using prototype = typename std::remove_reference<Function>::type;
			std::unique_ptr<detail::docker_interface> dck(new docker(this, factory<prototype, std::is_bind_expression<prototype>::value>::build(std::forward<Function>(fn)), false));
			auto evt = reinterpret_cast<event_handle>(dck.get());
			dockers_->emplace(dockers_->begin(), std::move(dck));
			detail::events_operation_register(evt);
			return evt;
		}

		event_handle connect(void (*fn)(arg_reference))
		{
			return connect([fn](arg_reference arg){
				fn(arg);
			});
		}

		/// It will not get called if stop_propagation() was called.
		template<typename Function>
		event_handle connect(Function && fn)
		{
			internal_scope_guard lock;
			if (nullptr == dockers_)
				dockers_.reset(new std::vector<std::unique_ptr<detail::docker_interface>>);

			using prototype = typename std::remove_reference<Function>::type;
			std::unique_ptr<detail::docker_interface> dck(new docker(this, factory<prototype, std::is_bind_expression<prototype>::value>::build(std::forward<Function>(fn)), false));
			auto evt = reinterpret_cast<event_handle>(dck.get());
			dockers_->emplace_back(std::move(dck));
			detail::events_operation_register(evt);
			return evt;
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
			internal_scope_guard lock;
			if (nullptr == dockers_)
				dockers_.reset(new std::vector<std::unique_ptr<detail::docker_interface>>);

			using prototype = typename std::remove_reference<Function>::type;
			std::unique_ptr<detail::docker_interface> dck(new docker(this, factory<prototype, std::is_bind_expression<prototype>::value>::build(std::forward<Function>(fn)), true));
			auto evt = reinterpret_cast<event_handle>(dck.get());
			if (in_front)
				dockers_->emplace(dockers_->begin(), std::move(dck));
			else
				dockers_->emplace_back(std::move(dck));
			detail::events_operation_register(evt);
			return evt;
		}

		std::size_t length() const
		{
			internal_scope_guard lock;
			return (nullptr == dockers_ ? 0 : dockers_->size());
		}

		void emit(arg_reference& arg)
		{
			internal_scope_guard lock;
			if (nullptr == dockers_)
				return;

			emit_counter ec(this);

			auto& dockers = *dockers_;
			const auto dockers_len = dockers.size();

			//The dockers may resize when a new event handler is created by a calling handler.
			//Traverses with position can avaid crash error which caused by a iterator which becomes invalid.
			for (std::size_t pos = 0; pos < dockers_len; ++pos)
			{
				auto docker_ptr = static_cast<docker*>(dockers[pos].get());
				if (docker_ptr->flag_deleted)
					continue;

				docker_ptr->invoke(arg);
				if (arg.propagation_stopped())
				{
					for (++pos; pos < dockers_len; ++pos)
					{
						auto docker_ptr = static_cast<docker*>(dockers[pos].get());
						if (!docker_ptr->unignorable || docker_ptr->flag_deleted)
							continue;

						docker_ptr->invoke(arg);
					}
					break;
				}
			}
		}

		void clear()
		{
			internal_scope_guard lock;
			if (dockers_)
				dockers_.reset();
		}

		void remove(event_handle evt) override
		{
			internal_scope_guard lock;
			if (dockers_)
			{
				for (auto i = dockers_->begin(), end = dockers_->end(); i != end; ++i)
				{
					if (reinterpret_cast<detail::docker_interface*>(evt) == i->get())
					{
						//Checks whether this event is working now.
						if (emitting_count_ > 1)
						{
							static_cast<docker*>(i->get())->flag_deleted = true;
							deleted_flags_ = true;
						}
						else
							dockers_->erase(i);
						break;
					}
				}
			}
		}
	private:
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

			static std::function<void(arg_reference)> build_second(fn_type&& fn, void(fn_type::*)(arg_reference))
			{
				return std::move(fn);
			}

			template<typename Tfn, typename Ret>
			static std::function<void(arg_reference)> build_second(Tfn&& fn, Ret(fn_type::*)()const)
			{
				return [fn](arg_reference) mutable
				{
					fn();
				};
			}

			static std::function<void(arg_reference)> build_second(fn_type&& fn, void(fn_type::*)(arg_reference) const)
			{
				return std::move(fn);
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

			static std::function<void(arg_reference)> build(Ret(*fn)(Arg))
			{
				return[fn](arg_reference arg){
					fn(arg);
				};
			}
		};
	private:
		unsigned emitting_count_{ 0 };
		bool deleted_flags_{ false };
		std::unique_ptr<std::vector<std::unique_ptr<detail::docker_interface>>> dockers_;
	};

	struct arg_mouse
		: public event_arg
	{
		event_code evt_code; ///< 
		::nana::window window_handle;  ///< A handle to the event window
		::nana::point pos;   ///< cursor position in the event window
		::nana::mouse button;	///< indicates a button which triggers the event

		bool left_button;	///< mouse left button is pressed?
		bool mid_button;	///< mouse middle button is pressed?
		bool right_button;	///< mouse right button is pressed?
		bool alt;			///< keyboard alt is pressed?
		bool shift;			///< keyboard Shift is pressed?
		bool ctrl;			///< keyboard Ctrl is pressed?

		/// Checks if left button is operated,
		bool is_left_button() const
		{
			return (event_code::mouse_move == evt_code ? left_button : (mouse::left_button == button));
		}
	};

    /// in arg_wheel event_code is event_code::mouse_wheel 
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
		::nana::window	window_handle;	    ///<  A handle to the event window
		::nana::point	pos;	            ///<  cursor position in the event window
		std::vector<std::string>	files;	///<  external filenames
	};

	struct arg_expose : public event_arg
	{
		::nana::window window_handle;	///< A handle to the event window
		bool exposed;	                ///< the window is visible?
	};

	struct arg_focus : public event_arg
	{
		::nana::window window_handle;	      ///< A handle to the event window
		::nana::native_window_type receiver;  ///< it is a native window handle, and specified which window receives focus
		bool getting;	                      ///< the window received focus?
	};

	struct arg_keyboard : public event_arg
	{
		event_code evt_code;	    ///< it is event_code::key_press in current event
		::nana::window window_handle;	///< A handle to the event window
		mutable wchar_t key;	///< the key corresponding to the key pressed
		mutable bool ignore;	    ///< this member is not used
		bool ctrl;	                ///< keyboard Ctrl is pressed?
		bool shift;	                ///< keyboard Shift is pressed
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

	struct arg_click : public event_arg
	{
		::nana::window window_handle;	///< A handle to the event window
		const arg_mouse* mouse_args;	///< If it is not null, it refers to the mouse arguments for click event emitted by mouse, nullptr otherwise.
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

#endif
