#ifndef NANA_GUI_FILEBOX_HPP
#define NANA_GUI_FILEBOX_HPP
#include <nana/gui/wvl.hpp>

namespace nana
{       /// Create an Open or Save dialog box to let user select the name of a file.
	class filebox
		: nana::noncopyable
	{
		struct implement;
	public:
		typedef std::vector<std::pair<nana::string, nana::string>> filters;

		filebox(window owner, bool is_open_mode);
		~filebox();

		/**	@brief	specify a title for the dialog
		 *	@param	string	a text for title
		 */
		nana::string title( nana::string new_title); ///< . Set a new title for the dialog and \return the old title

		/**	@brief	specify a suggestion directory
		 *	@param	string	a path of initial directory
		 *	@note	the behavior of init_path is different between Win7 and Win2K/XP/Vista, but its behavior under Linux is conformed with Win7.
		 */
		filebox& init_path(const nana::string&); ///< Suggested init path used to locate a directory when the filebox starts.
		filebox& init_file(const nana::string&); ///< Init file, if it contains a path, the init path is replaced by the path of init file.
        /// \brief Add a filetype filter. 
        /// To specify multiple filter in a single description, use a semicolon to separate the patterns(for example,"*.TXT;*.DOC;*.BAK").
		filebox& add_filter(const nana::string& description,  ///< for example. "Text File"
                            const nana::string& filetype      ///< filter pattern(for example, "*.TXT")
                            );

        filebox& add_filter(const filters &ftres)
        {
            for (auto &f : ftres)
                add_filter(f.first, f.second);
            return *this;
        };


		nana::string path() const;
		nana::string file() const;

		/**	@brief	Display the filebox dialog
		 */
		bool show() const;
		
		/** @brief	Display the filebox dialog
		 *	@note	A function object method alternative to show()
		 */
		bool operator()() const
		{
			return show();
		}
	private:
		implement * impl_;
	};
}//end namespace nana
#endif
