/**
 *	A group widget implementation
 *	Nana C++ Library(http://www.nanaro.org)
 *	Copyright(C) 2015-2018 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/widgets/group.hpp
 *
 *	@Author: Stefan Pfeifer(st-321), Ariel Vina-Rodriguez (qPCR4vir)
 *
 *	@brief group is a widget used to visually group and layout other widgets.
 */

#ifndef NANA_GUI_WIDGETS_GROUP_HPP
#define NANA_GUI_WIDGETS_GROUP_HPP

#include <nana/gui/place.hpp>
#include <nana/gui/widgets/panel.hpp>
#include <nana/gui/widgets/checkbox.hpp>

namespace nana{

	namespace drawerbase
	{
		namespace group
		{
			struct scheme : public nana::widget_geometrics
			{
				color_proxy border{ colors::gray_border };
			};
		}// end namespace panel
	}//end namespace drawerbase

	class group
		: public widget_object<category::widget_tag,
		                       drawerbase::panel::drawer,
		                       general_events,
		                       drawerbase::group::scheme>
	{
		struct implement;
	public:
		using field_reference = place::field_reference;
		constexpr static const std::size_t npos = static_cast<std::size_t>(-1);

		enum class background_mode
		{
			none,
			transparent,
			blending
		};

		/// The default construction
		group();

		/// The construction that creates the widget
		group(window parent, const rectangle& = {}, bool visible = true);

		///  The construction that creates the widget and set the title or caption
		group(window			parent,		        ///< a handle to the parent
			  ::std::string	    title,		        ///< caption of the group
			  bool				formatted = false,  ///< Enable/disable the formatted text for the title
			  unsigned			gap = 2,			///< between the content and the external limit
			  const rectangle&	r = {} ,
			  bool				visible = true
		     );


		/// The destruction
		~group();

		/// Adds an option for user selection
		checkbox& add_option(::std::string);

		/// Modifies the alignment of the title
		group& caption_align(align position);
		group& caption_background_mode(background_mode mode);

		/// Enables/disables the radio mode which is single selection
		group& radio_mode(bool);

		/// Returns the index of selected option in radio_mode, it throws a logic_error if radio_mode is false.
		std::size_t option() const;

        /** Set check/unchecked status of specified option
            @param[in] pos zero-based index of option to set
            @param[in] check status required, defaults to checked
            throws an out_of_range if !(pos < number of options)
        */
        void option_check( std::size_t pos, bool check = true );

		/// Determines whether a specified option is checked, it throws an out_of_range if !(pos < number of options)
		bool option_checked(std::size_t pos) const;

		/// Change typeface of caption label ( does not effect child widgets )
		void typeface( const nana::paint::font& font );

		group& enable_format_caption(bool format);

		group& collocate() noexcept;

		/// this will set the `usr_div_str` for an internal field, called the "user field".
		/// The "full" `place` of a `group` widget is internally divided into a field for the title,
		/// a field for the added "options" and a field for "user" widgets.
		group& div(const char* div_str) noexcept;
		field_reference operator[](const char* field);

		void field_display(const char* field_name, bool display); ///<Displays/Discards an existing field.
		bool field_display(const char* field_name) const;	///<Determines whether the specified field is displayed.
		void erase(window handle);				///< Erases a window from field.

		template<typename Widget, typename ...Args>
		Widget* create_child(const char* field, Args && ... args)
		{
			auto wdg = new Widget(handle(), std::forward<Args>(args)...);
			_m_add_child(field, wdg);
			return wdg;
		}
	private:
		void _m_add_child(const char* field, widget*);
		void _m_init();
		void _m_complete_creation() override;
		native_string_type _m_caption() const noexcept override;
		void _m_caption(native_string_type&&) override;
	private:
		std::unique_ptr<implement> impl_;
    };

}//end namespace nana
#endif
