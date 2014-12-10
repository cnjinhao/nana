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
		class element_interface
		{
		public:
			typedef paint::graphics & graph_reference;

			virtual ~element_interface()
			{}

			virtual bool draw(graph_reference, nana::color_t bgcolor, nana::color_t fgcolor, const nana::rectangle&, element_state) = 0;
		};

		class crook_interface
		{
		public:
			typedef paint::graphics & graph_reference;
			typedef checkstate	state;

			struct data
			{
				state	check_state;
				bool	radio;
			};
				
			virtual ~crook_interface()
			{}

			virtual bool draw(graph_reference, nana::color_t bgcolor, nana::color_t fgcolor, const nana::rectangle&, element_state, const data&) = 0;
		};

		class provider
		{
		public:
			template<typename ElementInterface>
			struct factory_interface
			{
				virtual ~factory_interface(){}
				virtual ElementInterface* create() const = 0;
				virtual void destroy(ElementInterface*) const = 0;
			};

			template<typename Element, typename ElementInterface>
			class factory
				: public factory_interface<ElementInterface>
			{
			public:
				typedef factory_interface<ElementInterface> interface_type;

				ElementInterface * create() const override
				{
					return (new Element);
				}

				void destroy(ElementInterface * p) const override
				{
					delete p;
				}
			};

			void add_crook(const std::string& name, const pat::cloneable<factory_interface<crook_interface>>&);
			crook_interface* const * keeper_crook(const std::string& name);
		};

		template<typename UserElement>
		void add_crook(const std::string& name)
		{
			typedef provider::factory<UserElement, crook_interface> factory_t;
			provider().add_crook(name, pat::cloneable<typename factory_t::interface_type>(factory_t()));
		}

		class crook;
	}//end namespace element

	template<typename Element> class facade;

	template<>
	class facade<element::crook>
		: public element::element_interface
	{
	public:
		typedef ::nana::paint::graphics & graph_reference;
		typedef element::crook_interface::state state;

		facade();
		facade(const char* name);

		facade & reverse();
		facade & check(state);
		state checked() const;

		facade& radio(bool);
		bool radio() const;

		void switch_to(const char*);
	public:
		//Implement element_interface
		bool draw(graph_reference, nana::color_t bgcolor, nana::color_t fgcolor, const nana::rectangle& r, element_state) override;
	private:
		element::crook_interface::data data_;
		element::crook_interface* const * keeper_;
	};

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

			bool draw(graph_reference, nana::color_t bgcolor, nana::color_t fgcolor, const nana::rectangle&, element_state);
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
			virtual bool draw(graph_reference, nana::color_t bgcolor, nana::color_t fgcolor, const nana::rectangle&, element_state);
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
