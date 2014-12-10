/*
*	Definition of General Events
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
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
#include <algorithm>

namespace nana
{
	namespace detail
	{
		class event_interface
		{
		public:
			virtual ~event_interface(){}
			virtual void remove(event_handle) = 0;
		};

		class docker_interface
		{
		public:
			virtual ~docker_interface(){}
			virtual event_interface*	get_event() const = 0;
		};

		class event_arg_interface
		{
		public:
			virtual ~event_arg_interface(){}
		};

		void events_operation_register(event_handle);
		void events_operation_cancel(event_handle);
	}//end namespace detail

	struct general_events;

	template<typename Arg>
	class basic_event : public detail::event_interface
	{
	public:
		typedef const typename std::remove_reference<Arg>::type & arg_reference;
	private:
		struct docker
			: public detail::docker_interface
		{
			basic_event * const event_ptr;
			std::function<void(arg_reference)> invoke;
			bool flag_entered = false;
			bool flag_deleted = false;

			docker(basic_event * s, std::function<void(arg_reference)> && ivk)
				: event_ptr(s), invoke(std::move(ivk))
			{}

			docker(basic_event * s, const std::function<void(arg_reference)> & ivk)
				: event_ptr(s), invoke(ivk)
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
	public:
		template<typename Function>
		event_handle connect_front(Function && fn)
		{
			internal_scope_guard lock;
			if (nullptr == dockers_)
				dockers_.reset(new std::vector<std::unique_ptr<docker>>);

			typedef typename std::remove_reference<Function>::type prototype;
			std::unique_ptr<docker> dck(new docker(this, factory<prototype, std::is_bind_expression<prototype>::value>::build(std::forward<Function>(fn))));
			auto evt = reinterpret_cast<event_handle>(static_cast<detail::docker_interface*>(dck.get()));
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

		template<typename Function>
		event_handle connect(Function && fn)
		{
			internal_scope_guard lock;
			if (nullptr == dockers_)
				dockers_.reset(new std::vector<std::unique_ptr<docker>>);

			typedef typename std::remove_reference<Function>::type prototype;
			std::unique_ptr<docker> dck(new docker(this, factory<prototype, std::is_bind_expression<prototype>::value>::build(std::forward<Function>(fn))));
			auto evt = reinterpret_cast<event_handle>(static_cast<detail::docker_interface*>(dck.get()));
			dockers_->emplace_back(std::move(dck));
			detail::events_operation_register(evt);
			return evt;
		}

		template<typename Function>
		event_handle operator()(Function&& fn)
		{
			return connect(std::forward<Function>(fn));
		}

		std::size_t length() const
		{
			internal_scope_guard lock;
			return (nullptr == dockers_ ? 0 : dockers_->size());
		}

		void emit(arg_reference& arg) const
		{
			internal_scope_guard lock;
			if (nullptr == dockers_)
				return;

			//Make a copy to allow create/destroy a new event handler when the call of emit in an event.
			const std::size_t fixed_size = 10;
			docker* fixed_buffer[fixed_size];
			docker** transitory = fixed_buffer;

			std::unique_ptr<docker*[]> variable_buffer;
			auto& dockers = *dockers_;
			if (dockers.size() > fixed_size)
			{
				variable_buffer.reset(new docker*[dockers.size()]);
				transitory = variable_buffer.get();
			}

			auto output = transitory;
			for (auto & dck : dockers)
			{
				(*output++) = dck.get();
			}

			for (; transitory != output; ++transitory)
			{
				std::unique_ptr<docker> p(*transitory);
				auto i = std::find(dockers.begin(), dockers.end(), p);
				if (i != dockers.end())
				{
					(*transitory)->flag_entered = true;
					(*transitory)->invoke(arg);
					(*transitory)->flag_entered = false;

					if ((*transitory)->flag_deleted)
						dockers.erase(i);
				}
				p.release();
			}
		}

		void clear()
		{
			internal_scope_guard lock;
			if (dockers_)
				dockers_.reset();
		}

		void remove(event_handle evt)
		{
			internal_scope_guard lock;
			if (dockers_)
			{
				auto i = std::find_if(dockers_->begin(), dockers_->end(), [evt](const std::unique_ptr<docker>& sp)
				{
					return (reinterpret_cast<detail::docker_interface*>(evt) == sp.get());
				});

				if (i != dockers_->end())
				{
					if (i->get()->flag_entered)
						i->get()->flag_deleted = true;
					else
						dockers_->erase(i);
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
		std::unique_ptr<std::vector<std::unique_ptr<docker>>> dockers_;
	};

	struct arg_mouse
		: public detail::event_arg_interface
	{
		event_code evt_code;
		::nana::window window_handle;
		::nana::point pos;
		bool left_button;
		bool mid_button;
		bool right_button;
		bool shift;
		bool ctrl;
	};

	struct arg_wheel : public arg_mouse
	{
		enum class wheel{
			vertical,
			horizontal
		};

		wheel	which;		///<which wheel is rotated
		bool	upwards;	///< true if the wheel is rotated to the top/left, depends on which. false otherwise.
		unsigned distance;	//expressed in multiples or divisions of 120
	};

	struct arg_dropfiles : public detail::event_arg_interface
	{
		::nana::window	window_handle;
		::nana::point	pos;
		std::vector<nana::string>	files;
	};

	struct arg_expose : public detail::event_arg_interface
	{
		::nana::window window_handle;
		bool exposed;
	};

	struct arg_focus : public detail::event_arg_interface
	{
		::nana::window window_handle;
		::nana::native_window_type receiver;
		bool getting;
	};

	struct arg_keyboard : public detail::event_arg_interface
	{
		event_code evt_code;
		::nana::window window_handle;
		mutable nana::char_t key;
		mutable bool ignore;
		bool ctrl;
		bool shift;
	};

	struct arg_move : public detail::event_arg_interface
	{
		::nana::window window_handle;
		int x;
		int y;
	};

	struct arg_resized : public detail::event_arg_interface
	{
		::nana::window window_handle;
		unsigned width;
		unsigned height;
	};

	struct arg_resizing : public detail::event_arg_interface
	{
		::nana::window window_handle;
		window_border border;
		mutable unsigned width;
		mutable unsigned height;
	};

	struct arg_unload : public detail::event_arg_interface
	{
		::nana::window window_handle;
		mutable bool cancel;
	};

	struct arg_destroy : public detail::event_arg_interface
	{
		::nana::window window_handle;
	};

	struct general_events
	{
		virtual ~general_events(){}
		basic_event<arg_mouse> mouse_enter;
		basic_event<arg_mouse> mouse_move;
		basic_event<arg_mouse> mouse_leave;
		basic_event<arg_mouse> mouse_down;
		basic_event<arg_mouse> mouse_up;
		basic_event<arg_mouse> click;
		basic_event<arg_mouse> dbl_click;
		basic_event<arg_wheel> mouse_wheel;
		basic_event<arg_dropfiles>	mouse_dropfiles;
		basic_event<arg_expose>	expose;
		basic_event<arg_focus>	focus;
		basic_event<arg_keyboard>	key_press;
		basic_event<arg_keyboard>	key_release;
		basic_event<arg_keyboard>	key_char;
		basic_event<arg_keyboard>	shortkey;

		basic_event<arg_move>		move;
		basic_event<arg_resizing>	resizing;
		basic_event<arg_resized>	resized;
		basic_event<arg_destroy>	destroy;
	};

	namespace detail
	{
		struct events_root_extension
			: public general_events
		{
			basic_event<arg_unload>	unload;
		};
	}//end namespace detail

	namespace dev
	{
		template<typename Widget>
		struct event_mapping
		{
			typedef general_events type;
		};
	}//end namespace dev

}//end namespace nana

#endif
