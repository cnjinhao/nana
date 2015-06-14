/**
 *	A Inline Widget Manager Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2015 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/detail/inline_widget_manager.hpp
 *
 */

#ifndef NANA_GUI_INLINE_WIDGET_MANAGER_HPP
#define NANA_GUI_INLINE_WIDGET_MANAGER_HPP
#include "inline_widget.hpp"
#include <nana/pat/abstract_factory.hpp>
#include "../panel.hpp"

#include <vector>

namespace nana
{
	namespace detail
	{

		template<typename Index, typename Value>
		class inline_widget_manager
		{
			using index_type = Index;
			using value_type = Value;
			using indicator_type = inline_widget_indicator<value_type>;
			using inline_widget = inline_widget_interface<index_type, value_type>;
			using factory = pat::abstract_factory<inline_widget>;

			struct widget_holder
			{
				panel<false> docker;
				std::unique_ptr<inline_widget> widget_ptr;
			};
		public:
			void set_window(window wd)
			{
				window_handle_ = wd;
			}

			void set_factory(std::unique_ptr<factory> fac)
			{
				factory_.swap(fac);
			}

			void place(point pos, const size& dimension, const size & visible_size, const indicator_type& indicator, index_type index)
			{
				auto holder = _m_append();
				holder->docker.move({ pos, visible_size });
				holder->widget_ptr->move({ point(), dimension });

				holder->widget_ptr->activate(indicator, index);
			}
		private:
			widget_holder* _m_append()
			{
				if (swap_widgets_.empty())
				{
					widgets_.emplace_back();
					widgets_.back().swap(swap_widgets_.back());
					swap_widgets_.pop_back();
				}
				else
				{

					widgets_.emplace_back(new widget_holder);
					auto & holder = widgets_.back();
					holder->docker.create(window_handle_, false);
					holder->widget_ptr.swap(factory_->create());
					holder->widget_ptr->create(holder->docker->handle());
				}
				return widgets_.back().get();
			}
		private:
			window	window_handle_{nullptr};
			std::unique_ptr<factory> factory_;
			std::vector<std::unique_ptr<widget_holder>> widgets_;
			std::vector<std::unique_ptr<widget_holder>> swap_widgets_;
		};
	}
}

#endif
