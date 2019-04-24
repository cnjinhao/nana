/**
 *	A Tree Box Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *   @file   nana/gui/widgets/treebox.hpp
 *	 @brief
 *		The treebox organizes the nodes by a key string. 
 *		The treebox would have a vertical scrollbar if there are too many nodes
 *	    to display. It does not have an horizontal scrollbar:
 *	    the widget will adjust the node's displaying position for fitting.
 */

#ifndef NANA_GUI_WIDGETS_TREEBOX_HPP
#define NANA_GUI_WIDGETS_TREEBOX_HPP

#include <nana/push_ignore_diagnostic>
#include "widget.hpp"
#include "detail/compset.hpp"
#include "detail/tree_cont.hpp"
#include <nana/gui/timer.hpp>
#include <nana/any.hpp>
#include <nana/pat/cloneable.hpp>
#include <stdexcept>

namespace nana
{
	class treebox;

	namespace drawerbase
	{
		namespace treebox
		{
			enum class component
			{
				begin, expander = begin, crook, icon, text, bground, end
			};

			struct node_image_tag
			{
				nana::paint::image normal;
				nana::paint::image hovered;
				nana::paint::image expanded;
			};

			struct node_attribute
			{
				bool has_children;
				bool expended;
				checkstate checked;
				bool selected;
				bool mouse_pointed;
				nana::paint::image icon_normal;
				nana::paint::image icon_hover;
				nana::paint::image icon_expanded;
				::std::string text;
			};

			struct scheme
				: public widget_geometrics
			{
				color_proxy item_bg_selected{ static_cast<color_rgb>(0xD5EFFC) };  ///< item selected: background color
				color_proxy item_fg_selected{ static_cast<color_rgb>(0x99DEFD) };  ///< item selected: foreground color
				color_proxy item_bg_highlighted{ static_cast<color_rgb>(0xE8F5FD) };  ///< item highlighted: background color
				color_proxy item_fg_highlighted{ static_cast<color_rgb>(0xD8F0FA) };  ///< item highlighted: foreground color
				color_proxy item_bg_selected_and_highlighted{ static_cast<color_rgb>(0xC4E8FA) };  ///< item selected and highlighted: background color
				color_proxy item_fg_selected_and_highlighted{ static_cast<color_rgb>(0xB6E6FB) };  ///< item selected and highlighted: foreground color

				unsigned item_offset{ 16 }; ///< item position displacement in pixels
				unsigned text_offset{ 4 }; ///< text position displacement in pixels
				unsigned icon_size{ 16 }; ///< icon size in pixels
				unsigned crook_size{ 16 }; ///< crook size in pixels (TODO: the function that draw the crook doesn't scale the shape)

				unsigned indent_displacement{ 18 }; ///< children position displacement in pixels (def=18 (before was 10))
			};

			typedef widgets::detail::compset<component, node_attribute> compset_interface;
			typedef widgets::detail::compset_placer<component, node_attribute, scheme> compset_placer_interface;
			
			class renderer_interface
			{
			public:
				typedef drawerbase::treebox::component component;
				typedef ::nana::paint::graphics& graph_reference;
				typedef drawerbase::treebox::compset_interface compset_interface;
				typedef compset_interface::item_attribute_t item_attribute_t;
				typedef compset_interface::comp_attribute_t comp_attribute_t;

				virtual ~renderer_interface() = default;

				virtual void begin_paint(::nana::widget&) = 0;
				virtual void bground(graph_reference, const compset_interface *) const = 0;
				virtual void expander(graph_reference, const compset_interface *) const = 0;
				virtual void crook(graph_reference, const compset_interface *) const = 0;
				virtual void icon(graph_reference, const compset_interface *) const = 0;
				virtual void text(graph_reference, const compset_interface *) const = 0;
			};

			class item_proxy;

			class trigger
				:public drawer_trigger
			{
				class implementation;
				class item_locator;
			public:
				struct treebox_node_type
				{
					treebox_node_type();
					treebox_node_type(std::string);
					treebox_node_type& operator=(const treebox_node_type&);

					::std::string text;
					nana::any value;
					bool expanded;
					checkstate checked;
					::std::string img_idstr;
				};

				struct pseudo_node_type{};

				using tree_cont_type = widgets::detail::tree_cont<treebox_node_type>;
				using node_type = tree_cont_type::node_type;

				trigger();
				~trigger();

				implementation * impl() const;

				void check(node_type*, checkstate);

				pat::cloneable<renderer_interface>& renderer() const;

				void placer(::nana::pat::cloneable<compset_placer_interface>&&);
				const ::nana::pat::cloneable<compset_placer_interface>& placer() const;

				node_type* insert(node_type*, const std::string& key, std::string&&);
				node_type* insert(const std::string& path, std::string&&);

				node_image_tag& icon(const ::std::string&);
				void icon_erase(const ::std::string&);
				void node_icon(node_type*, const ::std::string& id);
				unsigned node_width(const node_type*) const;

				bool rename(node_type*, const char* key, const char* name);

			private:
				//Overrides drawer_trigger methods
				void attached(widget_reference, graph_reference)		override;
				void detached() override;
				void refresh(graph_reference)	override;
				void dbl_click(graph_reference, const arg_mouse&)	override;
				void mouse_down(graph_reference, const arg_mouse&)	override;
				void mouse_up(graph_reference, const arg_mouse&)	override;
				void mouse_move(graph_reference, const arg_mouse&)	override;
				void mouse_wheel(graph_reference, const arg_wheel&)	override;
				void mouse_leave(graph_reference, const arg_mouse&)	override;
				void resized(graph_reference, const arg_resized&)		override;
				void key_press(graph_reference, const arg_keyboard&)	override;
				void key_char(graph_reference, const arg_keyboard&)	override;
			private:
				implementation * const impl_;
			}; //end class trigger


			/// \brief A proxy for accessing the node. The key string is case sensitive.
			class item_proxy
				: public std::iterator<std::input_iterator_tag, item_proxy>
			{
			public:
				item_proxy() = default;           ///< The default constructor creates an end iterator.

				//Undocumented constructor.
				item_proxy(trigger*, trigger::node_type*);

				/// Append a child.
				item_proxy append(const ::std::string& key, ::std::string name);

				/// Append a child with a specified value (user object.).
				template<typename T>
				item_proxy append(const ::std::string& key, ::std::string name, T&& t)
				{
					item_proxy ip = append(key, std::move(name));
					if(false == ip.empty())
						ip.value(std::forward<T>(t));
					return ip;
				}

				/// Return true if the proxy does not refer to a node, as an end iterator.
				bool empty() const;

				/// \brief Return the distance between the ROOT node and this node.
				/// @return  only available when empty() is false.
				std::size_t level() const;

				/// Return the check state
				bool checked() const;

				/// Set the check state, and it returns itself.
				item_proxy& check(bool);

				/// Clears the child nodes
				item_proxy& clear();

				/// Return true when the node is expanded  \todo change to expanded ??
				bool expanded() const;

				/// Expand/Shrink children of the node, and returns itself.  \todo change to expand ??
				item_proxy& expand(bool);

				/// Return true when the node is selected.
				bool selected() const;

				/// Select the node, and returns itself..
				item_proxy& select(bool);

				/// Return the icon.
				const ::std::string& icon() const;

				/// Set the icon, and returns itself..
				item_proxy& icon(const ::std::string& id);

				/// Return the text.
				const ::std::string& text() const;

				/// Set the text, and returns itself.
				item_proxy& text(const ::std::string&);

				/// Set a new key, and returns itself..
				item_proxy& key(const ::std::string& s);

				/// Return the key.
				const ::std::string& key() const;

				std::size_t size() const; ///< Returns the number of child nodes.

				/// Return the first child of the node.
				item_proxy child() const;

				/// Return the owner of the node.
				item_proxy owner() const;

				/// Return the sibling of the node.
				item_proxy sibling() const;

				/// Return the first child of the node
				item_proxy begin() const;

				/// An end node.
				item_proxy end() const;

				/// Makes an action for each sub item recursively, returns the item that stops the action where action returns false.
				item_proxy visit_recursively(std::function<bool(item_proxy)> action);

				bool operator==(const ::std::string& s) const; ///< Compare the text of node with s.
				bool operator==(const char* s ) const;        ///< Compare the text of node with s.
				bool operator==(const wchar_t* s ) const;     ///< Compare the text of node with s.

				/// Behavior of Iterator
				item_proxy& operator=(const item_proxy&);

				/// Behavior of Iterator
				item_proxy & operator++();

				/// Behavior of Iterator
				item_proxy	operator++(int);

				/// Behavior of Iterator
				item_proxy& operator*();

				/// Behavior of Iterator
				const item_proxy& operator*() const;

				/// Behavior of Iterator
				item_proxy* operator->();

				/// Behavior of Iterator
				const item_proxy* operator->() const;

				/// Behavior of Iterator
				bool operator==(const item_proxy&) const;

				/// Behavior of Iterator
				bool operator!=(const item_proxy&) const;

				template<typename T>
				const T * value_ptr() const
				{
					return any_cast<T>(&_m_value());
				}

				template<typename T>
				const T& value() const
				{
					auto p = any_cast<T>(&_m_value());
					if(nullptr == p)
						throw std::runtime_error("treebox::value<T>() Invalid type of value.");
					return *p;
				}

				template<typename T>
				item_proxy & value(T&& t)
				{
					_m_value() = std::forward<T>(t);
					return *this;
				}

				// Undocumented methods for internal use
				trigger::node_type * _m_node() const;
			private:
				nana::any& _m_value();
				const nana::any& _m_value() const;
			private:
				trigger * trigger_{nullptr};
				trigger::node_type * node_{nullptr};
			};//end class item_proxy
		}//end namespace treebox
	}//end namespace drawerbase

    ///  a type of treebox event parameter
	struct arg_treebox 
		: public event_arg
	{
		treebox& widget;                          ///< where the event occurs
		drawerbase::treebox::item_proxy & item;   ///< the operated node
		bool	operated;                         ///< operation state of the event

		arg_treebox(treebox&, drawerbase::treebox::item_proxy&, bool operated);
	};

	namespace drawerbase
	{
		namespace treebox
		{
			struct treebox_events
				: public general_events
			{
				basic_event<arg_treebox> expanded; ///< a user expands or shrinks a node
				basic_event<arg_treebox> checked;  ///< a user checks or unchecks a node
				basic_event<arg_treebox> selected; ///< a user selects or unselects a node
				basic_event<arg_treebox> hovered;  ///< a user moves the cursor over a node
			};
		}//end namespace treebox
	}//end namespace drawerbase

    /// \brief  Displays a hierarchical list of items, such as the files and directories on a disk.
    /// See also in [documentation](http://nanapro.org/en-us/documentation/widgets/treebox.htm)
    class treebox
		:public widget_object <category::widget_tag,
		                        drawerbase::treebox::trigger,
		                        drawerbase::treebox::treebox_events, drawerbase::treebox::scheme>
	{
	public:
        /// A type refers to the item and is also used to iterate through the nodes.
		typedef drawerbase::treebox::item_proxy	item_proxy;

        /// state images for the node
		typedef drawerbase::treebox::node_image_tag node_image_type;

		/// The interface of treebox user-defined item renderer
		typedef drawerbase::treebox::renderer_interface renderer_interface;

		/// The interface of treebox compset_placer to define the position of node components
		typedef drawerbase::treebox::compset_placer_interface compset_placer_interface;

		/// The default constructor without creating the widget.
		treebox();

		/// \brief The construct that creates a widget.
		/// @param wd  A handle to the parent window of the widget being created.
		/// @param visible  specifying the visibility after creating.
		treebox(window wd, bool visible);

		/// \brief  The construct that creates a widget.
		/// @param wd  A handle to the parent window of the widget being created.
		/// @param r  the size and position of the widget in its parent window coordinate.
		/// @param visible  specifying if visible after creating.
		treebox(window, const nana::rectangle& = rectangle(), bool visible = true);

		template<typename ItemRenderer>
		treebox & renderer(const ItemRenderer & rd) ///< set user-defined node renderer
		{
			get_drawer_trigger().renderer() = ::nana::pat::cloneable<renderer_interface>{rd};
			return *this;
		}

		const nana::pat::cloneable<renderer_interface> & renderer() const;  ///< get user-defined node renderer

		template<typename Placer>
		treebox & placer(const Placer & r) ///< location of a node components
		{
			get_drawer_trigger().placer(::nana::pat::cloneable<compset_placer_interface>(r));
			return *this;
		}

		const nana::pat::cloneable<compset_placer_interface> & placer() const;

		/// \brief  Enable the widget to be draws automatically when it is operated.
        ///
        /// The treebox automatically redraws after certain operations, but, 
        /// under some circumstances, it is good to disable the automatic drawing mode, 
        /// for example, before adding nodes in a loop, disable the mode avoiding 
        /// frequent and useless refresh for better performance, and then, after 
        /// the operations, enable the automatic redraw mode again.
		/// @param enable bool  whether to enable.
		void auto_draw(bool enable);

		/// Prevents drawing during execution.
		template<typename Function>
		void avoid_drawing(Function fn)
		{
			this->auto_draw(false);
			try
			{
				fn();
			}
			catch (...)
			{
				this->auto_draw(true);
				throw;
			}
			this->auto_draw(true);
		}

		/// \brief  Enable the checkboxs for each item of the widget.
		/// @param enable bool  indicates whether to show or hide the checkboxs.
		treebox & checkable(bool enable);

		
		bool checkable() const; ///< Are the checkboxs are enabled?

		/// Clears the contents
		void clear();

        /// \brief Creates an icon scheme with the specified name.
        ///
        /// The icon scheme includes 3 images for node states. 
        /// These states are 'normal', 'hovered' and 'expanded'. 
        /// If 'hovered' or 'expanded' are not set, it uses 'normal' state image for these 2 states.
        /// See also in [documentation](http://nanapro.org/en-us/help/widgets/treebox.htm)
		/// @param id The name of an icon scheme. If the name is not existing, it creates a new scheme for the name.
		/// @return The reference of node image scheme corresponding with the specified id.
		node_image_type& icon(const ::std::string& id);

		void icon_erase(const ::std::string& id);

		item_proxy find(const ::std::string& keypath);  ///< Find an item through a specified keypath.

        /// Inserts a new node to treebox, but if the keypath exists change and returns the existing node.
		item_proxy insert(const ::std::string& path_key,   ///< specifies the node hierarchy
                           ::std::string title      ///< used for displaying
                           ); 

        /// Inserts a new node to treebox, but if the keypath exists change and returns the existing node.
		item_proxy insert( item_proxy pos,             ///< the parent item node
                           const ::std::string& key,    ///< specifies the new node
                           ::std::string title   ///< title used for displaying in the new node.
                           );

		item_proxy erase(item_proxy i); ///< Removes the node at i and return the Item proxy following the removed node

		void erase(const ::std::string& keypath); ///< Removes the node by the key path. 

		::std::string make_key_path(item_proxy i, const ::std::string& splitter) const;///<returns the key path

		item_proxy selected() const; ///< returns the selected node

		/// Scrolls a specified item into view.
		/**
		 * @param item An item to be requested.
		 * @param bearing The position where the item to be positioned in the view.
		 */
		void scroll_into_view(item_proxy item, align_v bearing);

		/// Scrolls a specified item into view.
		/**
		 * @param item An item to be requested.
		 */
		void scroll_into_view(item_proxy item);

		/// Gets the current hovered node.
		item_proxy hovered(bool exclude_expander) const;
	private:
		std::shared_ptr<scroll_operation_interface> _m_scroll_operation() override;

	};//end class treebox
}//end namespace nana

#include <nana/pop_ignore_diagnostic>

#endif
