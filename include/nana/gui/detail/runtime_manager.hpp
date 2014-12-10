/*
 *	A Runtime Manager Implementation
 *	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/detail/runtime_manager.hpp
 *
 */
#ifndef NANA_GUI_DETAIL_RUNTIME_MANAGER_HPP
#define NANA_GUI_DETAIL_RUNTIME_MANAGER_HPP

#include <map>

namespace nana
{
	namespace detail
	{
		template<typename Window, typename Bedrock>
		class runtime_manager
		{
		public:
			typedef Window	window_handle;

			template<typename Form, typename... Args>
			Form* create_form(Args&&... args)
			{
				widget_placer<Form> * holder = new widget_placer<Form>;
				if (holder->create(std::forward<Args>(args)...))
				{
					holder_[holder->get_handle()] = holder;
					return holder->get();
				}

				delete holder;
				return nullptr;
			}

			void remove_if_exists(window_handle wd)
			{
				auto i = holder_.find(wd);
				if(i != holder_.cend())
				{
					delete i->second;
					holder_.erase(i);
				}
			}
		private:
			class widget_holder
			{
			public:
				virtual ~widget_holder(){}
				virtual window_handle get_handle() const = 0;
			};

			template<typename Form>
			class widget_placer : public widget_holder
			{
			public:
				widget_placer()
					:	form_(nullptr)
				{}

				~widget_placer()
				{
					delete form_;
				}

				template<typename... Args>
				bool create(Args&&... args)
				{
					if (nullptr == form_)
						form_ = new Form(std::forward<Args>(args)...);

					return (form_ && !form_->empty());
				}

				Form* get() const
				{
					return form_;
				}

				window_handle get_handle() const override
				{
					return reinterpret_cast<window_handle>(form_ ? form_->handle() : nullptr);
				}
			private:
				Form * form_;
			};
		private:
			std::map<window_handle, widget_holder*>	holder_;
		}; //end class runtime_manager

	}//end namespace detail
}//end namespace nana


#endif
