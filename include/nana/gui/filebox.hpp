/*
 *	Filebox
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2003-2016 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/gui/filebox.hpp
 */

#ifndef NANA_GUI_FILEBOX_HPP
#define NANA_GUI_FILEBOX_HPP
#include <nana/gui/basis.hpp>
#include <vector>
#include <utility>

namespace nana
{       /// Create an Open or Save dialog box to let user select the name of a file.
	class filebox
	{
		struct implement;

		filebox(filebox&&) = delete;
		filebox& operator=(filebox&&) = delete;
	public:
		using filters = std::vector<std::pair< ::std::string, ::std::string>>;

		filebox(bool is_open_mode);
		filebox(window owner, bool is_open_mode);
		filebox(const filebox&);
		~filebox();

		filebox& operator=(const filebox&);

		/// Change owner window
		void owner(window);

		/// specify a title for the dialog
		/// @param	string	a text for title
		/// @return old title.
		::std::string title( ::std::string new_title); ///< . Set a new title for the dialog and \return the old title

		/**	@brief	specify a suggestion directory
		 *	@param	string	a path of initial directory
		 *	@note	the behavior of init_path is different between Win7 and Win2K/XP/Vista, but its behavior under Linux is conformed with Win7.
		 */
		filebox& init_path(const ::std::string&); ///< Suggested init path used to locate a directory when the filebox starts.
		filebox& init_file(const ::std::string&); ///< Init file, if it contains a path, the init path is replaced by the path of init file.
        /// \brief Add a filetype filter. 
        /// To specify multiple filter in a single description, use a semicolon to separate the patterns(for example,"*.TXT;*.DOC;*.BAK").
		filebox& add_filter(const ::std::string& description,  ///< for example. "Text File"
                            const ::std::string& filetype      ///< filter pattern(for example, "*.TXT")
                            );

        filebox& add_filter(const filters &ftres)
        {
            for (auto &f : ftres)
                add_filter(f.first, f.second);
            return *this;
        };


		::std::string path() const;
		::std::string file() const;

		/// Display the filebox dialog
		bool show() const;
		
		/// Display the filebox dialog
		/// A function object method alternative to show()
		bool operator()() const
		{
			return show();
		}
	private:
		implement * impl_;
	};
}//end namespace nana
#endif
