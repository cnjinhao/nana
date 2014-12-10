/*
*	A Message Box Class
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2013 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file: nana/gui/msgbox.hpp
*/

#ifndef NANA_GUI_MSGBOX_HPP
#define NANA_GUI_MSGBOX_HPP

#include <sstream>
#include <nana/gui/basis.hpp>

namespace nana
{
	/// Prefabricated modal dialog box (with text, icon and actions buttons) that inform and instruct the user.
	class msgbox
	{
	public:
		/// Identifiers of icons.
		enum icon_t{icon_none, icon_information, icon_warning, icon_error, icon_question};

		/// Identifiers of buttons.
		enum button_t{ok, yes_no, yes_no_cancel};

		/// Identifiers of buttons that a user clicked.
		enum pick_t{pick_ok, pick_yes, pick_no, pick_cancel};

		/// Default construct that creates a message box with default title and default button, the default button is OK.
		msgbox();

		/// Copy construct from an existing msgbox object.
		msgbox(const msgbox&);

		/// Assign from an existing msgbox object.
		msgbox& operator=(const msgbox&);

		/// Construct that creates a message box with a specified title and default button.
		msgbox(const nana::string&);

		/// Construct that creates a message box with an owner window and a specified title.
		msgbox(window, const nana::string&);

		/// Construct that creates a message box with an owner windoow, a specified title and buttons. 
		msgbox(window, const nana::string&, button_t);

		/// Sets an icon for informing user.
		msgbox& icon(icon_t);

		/// Clears the text message buffer.
		void clear();

		/// Writes a string to the buffer.
		msgbox & operator<<(const nana::string&);

		/// Writes a string to the buffer.
		msgbox & operator<<(const nana::char_t*);

		/// Writes a string to the buffer.
		msgbox & operator<<(const nana::charset&);

		// Calls a manipulator to the stream.
		msgbox & operator<<(std::ostream& (*)(std::ostream&));

		/// Write a streamizable object to the buffer.
		template<typename T>
		msgbox & operator<<(const T& t)
		{
			sstream_<<t;
			return *this;
		}

		/// \brief Displays the message that buffered in the stream.
		/// @return, the button that user clicked.
		pick_t show() const;

		/// A function object method alternative to show()
		pick_t operator()() const
		{
			return show();
		}
	private:
		std::stringstream sstream_;
		window wd_;
		nana::string title_;
		button_t button_;
		icon_t icon_;
	};
}//end namespace nana

#endif
