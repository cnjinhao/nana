/*
 *	Elements of GUI Gadgets
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2014 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0. 
 *	(See accompanying file LICENSE_1_0.txt or copy at 
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/element.hpp
 */
#ifndef NANA_GUI_ELEMENT_HPP
#define NANA_GUI_ELEMENT_HPP

#include <nana/paint/graphics.hpp>
#include <nana/pat/cloneable.hpp>
#include <vector>
#include <map>

namespace nana
{
	namespace paint
	{
		//forward declaration
		class image;
	}

	namespace element
	{
		namespace detail
		{
			class element_abstract
			{
			public:
				using graph_reference = ::nana::paint::graphics&;
				virtual ~element_abstract() = default;
			};

			class factory_abstract
			{
			public:
				virtual ~factory_abstract() = default;

				virtual void destroy(element_abstract *);
			};
		}

		class element_interface
			: public detail::element_abstract
		{
		public:
			virtual bool draw(graph_reference, const nana::color& bgcolor, const nana::color& fgcolor, const nana::rectangle&, element_state) = 0;
		};

		class crook_interface
			: public detail::element_abstract
		{
		public:
			using state = checkstate;

			struct data
			{
				state	check_state;
				bool	radio;
			};

			virtual bool draw(graph_reference, const nana::color& bgcolor, const nana::color& fgcolor, const nana::rectangle&, element_state, const data&) = 0;
		};

		class border_interface
			: public detail::element_abstract
		{
		public:
			virtual bool draw(graph_reference, const ::nana::color& bgcolor, const ::nana::color& fgcolor, const ::nana::rectangle&, element_state, unsigned weight) = 0;
		};

		class arrow_interface
			: public detail::element_abstract
		{
		public:
			virtual bool draw(graph_reference, const ::nana::color& bgcolor, const ::nana::color& fgcolor, const ::nana::rectangle&, element_state, direction) = 0;
		};

		class provider
		{
		public:
			template<typename ElementInterface>
			struct factory_interface
				: public detail::factory_abstract
			{
				virtual ~factory_interface(){}
				virtual ElementInterface* create() const = 0;
			};

			template<typename Element, typename ElementInterface>
			class factory
				: public factory_interface<ElementInterface>
			{
			public:
				using interface_type = factory_interface<ElementInterface>;

				ElementInterface * create() const override
				{
					return (new Element);
				}
			};

			void add_arrow(const std::string&, const pat::cloneable<factory_interface<arrow_interface>>&);
			arrow_interface* const * cite_arrow(const std::string&);

			void add_border(const std::string&, const pat::cloneable<factory_interface<border_interface>>&);
			border_interface* const * cite_border(const std::string&);

			void add_button(const std::string&, const pat::cloneable<factory_interface<element_interface>>&);
			element_interface* const* cite_button(const std::string&);

			void add_x_icon(const std::string& name, const pat::cloneable<factory_interface<element_interface>>&);
			element_interface* const* cite_x_icon(const std::string&);

			void add_crook(const std::string& name, const pat::cloneable<factory_interface<crook_interface>>&);
			crook_interface* const * cite_crook(const std::string& name);

			void add_cross(const std::string& name, const pat::cloneable<factory_interface<element_interface>>&);
			element_interface* const* cite_cross(const std::string&);
		};

		class arrow;
		template<typename ArrowElement>
		void add_arrow(const std::string& name)
		{
			using factory_t = provider::factory<ArrowElement, arrow_interface>;
			provider().add_arrow(name, pat::cloneable<typename factory_t::interface_type>(factory_t()));
		}

		class border;
		template<typename BorderElement>
		void add_border(const std::string& name)
		{
			using factory_t = provider::factory<BorderElement, border_interface>;
			provider().add_border(name, pat::cloneable<typename factory_t::interface_type>(factory_t()));
		}

		class button;
		template<typename ButtonElement>
		void add_button(const std::string& name)
		{
			using factory_t = provider::factory<ButtonElement, element_interface>;
			provider().add_button(name, pat::cloneable<typename factory_t::interface_type>(factory_t()));
		}

		class x_icon;
		template<typename UserElement>
		void add_x_icon(const std::string& name)
		{
			using factory_t = provider::factory<UserElement, element_interface>;
			provider().add_x_icon(name, pat::cloneable<typename factory_t::interface_type>(factory_t()));
		}

		class crook;
		template<typename UserElement>
		void add_crook(const std::string& name)
		{
			using factory_t = provider::factory<UserElement, crook_interface>;
			provider().add_crook(name, pat::cloneable<typename factory_t::interface_type>(factory_t()));
		}

		class cross;
		template<typename UserElement>
		void add_cross(const std::string& name)
		{
			using factory_t = provider::factory<UserElement, element_interface>;
			provider().add_cross(name, pat::cloneable<typename factory_t::interface_type>(factory_t()));
		}
	}//end namespace element

	template<typename Element> class facade;

	template<>
	class facade<element::crook>
		: public element::element_interface
	{
	public:
		using graph_reference = ::nana::paint::graphics &;
		using state = element::crook_interface::state;

		facade(const char* name = nullptr);

		facade & reverse();
		facade & check(state);
		state checked() const;

		facade& radio(bool);
		bool radio() const;

		void switch_to(const char*);
	public:
		//Implement element_interface
		bool draw(graph_reference, const nana::color& bgcolor, const nana::color& fgcolor, const nana::rectangle& r, element_state) override;
	private:
		element::crook_interface::data data_;
		element::crook_interface* const * cite_;
	}; //end class facade<element::crook>

	template<> class facade<element::cross>
		: public element::element_interface
	{
	public:
		facade(const char* name = nullptr);
		void switch_to(const char*);

		void thickness(unsigned thk);
		void size(unsigned size_pixels);
	public:
		//Implement element_interface
		bool draw(graph_reference, const ::nana::color& bgcolor, const ::nana::color& fgcolor, const ::nana::rectangle&, element_state) override;
	private:
		unsigned thickness_{6};
		unsigned size_{ 14 };
		element::element_interface* const * cite_;
	};

	template<>
	class facade<element::border>
		: public element::element_interface
	{
		using graph_reference = ::nana::paint::graphics &;
	public:
		facade(const char* name = nullptr);

		void switch_to(const char*);
	public:
		//Implement element_interface
		bool draw(graph_reference, const nana::color& bgcolor, const nana::color& fgcolor, const nana::rectangle&, element_state) override;
	private:
		element::border_interface* const * cite_;
	};//end class facade<element::border>

	template<>
	class facade<element::arrow>
		: public element::element_interface
	{
		using graph_reference = ::nana::paint::graphics &;
	public:
		enum class style
		{
			solid
		};

		facade(const char* name = nullptr);

		void switch_to(const char*);
		void direction(::nana::direction);
	public:
		//Implement element_interface
		bool draw(graph_reference, const nana::color& bgcolor, const nana::color& fgcolor, const nana::rectangle&, element_state) override;
	private:
		element::arrow_interface* const * cite_;
		::nana::direction dir_{::nana::direction::north};
	};//end class facade<element::arrow>

	template<>
	class facade<element::button>
		: public element::element_interface
	{
	public:
		facade(const char* name = nullptr);
		void switch_to(const char*);
	public:
		//Implement element_interface
		bool draw(graph_reference, const ::nana::color& bgcolor, const ::nana::color& fgcolor, const ::nana::rectangle&, element_state) override;
	private:
		element::element_interface* const * cite_;
	};//end class facade<element::button>

	template<>
	class facade<element::x_icon>
		: public element::element_interface
	{
	public:
		facade(const char* name = nullptr);
		void switch_to(const char*);
	public:
		//Implement element_interface
		bool draw(graph_reference, const ::nana::color& bgcolor, const ::nana::color& fgcolor, const ::nana::rectangle&, element_state) override;
	private:
		element::element_interface* const * cite_;
	};//end class facade<element::button>

	namespace element
	{
		void set_bground(const char* name, const pat::cloneable<element_interface>&);
		void set_bground(const char* name, pat::cloneable<element_interface> &&);

		class cite_bground
		{
		public:
			typedef paint::graphics& graph_reference;
			typedef pat::cloneable<element_interface> cloneable_element;

			cite_bground(const char*);

			void set(const cloneable_element&);
			void set(const char*);

			bool draw(graph_reference, const nana::color& bgcolor, const nana::color& fgcolor, const nana::rectangle&, element_state);
		private:
			cloneable_element holder_;
			element_interface * place_ptr_;
			element_interface * const * ref_ptr_;
		};

		class bground
			: public element_interface
		{
		public:
			typedef paint::graphics& graph_reference;

			bground();
			bground(const bground&);
			~bground();
			bground& operator=(const bground&);

			bground& image(const paint::image&, bool vertical, const nana::rectangle& valid_area);		///< Set a picture for the background
			bground& image(const paint::graphics&, bool vertical, const nana::rectangle& valid_area);	///< Set a picture for the background

			void states(const std::vector<element_state> &);	///< Set the state sequence of the background picture.
			void states(std::vector<element_state> &&);			///< Set the state sequence of the background picture.
			void reset_states();

			void join(element_state target, element_state joiner);

			void stretch_parts(unsigned left, unsigned top, unsigned right, unsigned bottom);

			//Implement the methods of element_interface.
			virtual bool draw(graph_reference, const nana::color& bgcolor, const nana::color& fgcolor, const nana::rectangle&, element_state);
		private:
			struct draw_method;
			struct draw_image;
			struct draw_graph;

			draw_method * method_;
			bool	vertical_;
			nana::rectangle valid_area_;
			std::vector<element_state> states_;
			std::map<element_state, element_state> join_;
			bool	stretch_all_;
			unsigned left_, top_, right_, bottom_;
		}; //end class bground
	}//end namespace element
}//end namespace nana

#endif	//NANA_GUI_ELEMENT_HPP
