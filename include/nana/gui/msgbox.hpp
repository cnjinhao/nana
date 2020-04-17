/**
*	A Message Box Class
*	Nana C++ Library(http://www.nanapro.org)
*	Copyright(C) 2003-2019 Jinhao(cnjinhao@hotmail.com)
*
*	Distributed under the Boost Software License, Version 1.0.
*	(See accompanying file LICENSE_1_0.txt or copy at
*	http://www.boost.org/LICENSE_1_0.txt)
*
*	@file nana/gui/msgbox.hpp
*/

#ifndef NANA_GUI_MSGBOX_HPP
#define NANA_GUI_MSGBOX_HPP
#include <nana/push_ignore_diagnostic>

#include <sstream>

namespace nana
{
	//Forward declaration of filebox for msgbox
	class filebox;

	/// Prefabricated modal dialog box (with text, icon and actions buttons) that informs and instructs the user.
	class msgbox
	{
	public:
		/// Identifiers of icons.
		enum icon_t{icon_none, icon_information, icon_warning, icon_error, icon_question};

		/// Identifiers of buttons.
		enum button_t{ok, yes_no, yes_no_cancel};

		/// Identifiers of buttons that a user clicked.
		enum pick_t{pick_ok, pick_yes, pick_no, pick_cancel};


		/// Default constructor that creates a message box with default title and default button, the default button is OK.
		msgbox();

		/// Copy constructor from an existing msgbox object.
		msgbox(const msgbox&);

		/// Assign from an existing msgbox object.
		msgbox& operator=(const msgbox&);

		/// Constructor that creates a message box with a specified title and default button.
		msgbox(const ::std::string&);

		/// Constructor that creates a message box with an owner window, a specified title and buttons.
		msgbox(window, const ::std::string&, button_t = ok);

#ifdef __cpp_char8_t
		msgbox(std::u8string_view);
		msgbox(window, std::u8string_view, button_t = ok);
#endif

		/// Sets an icon for informing user.
		msgbox& icon(icon_t);

		/// Clears the text message buffer.
		void clear();

		// Calls a manipulator to the stream.
		msgbox & operator<<(std::ostream& (*)(std::ostream&));

		/// Write a streamable object to the buffer.
		template<typename T>
		msgbox & operator<<(T&& t)
		{
			using parameter_type = std::remove_cv_t<std::remove_pointer_t<std::decay_t<T>>>;

			if constexpr(std::is_same_v<parameter_type, std::wstring> || std::is_same_v<parameter_type, wchar_t> || std::is_same_v<parameter_type, std::wstring_view>)
			{
				if constexpr (std::is_same_v<std::remove_cv_t<T>, wchar_t>)
					sstream_ << to_utf8(std::wstring_view{&t, 1});
				else
					sstream_ << to_utf8(t);
			}
			else if constexpr(std::is_same_v<parameter_type, std::string> || std::is_same_v<parameter_type, char> || std::is_same_v<parameter_type, std::string_view>)
			{
				if constexpr (!std::is_same_v<std::remove_cv_t<T>, wchar_t>)
					review_utf8(t);

				sstream_ << t;
			}
			else if constexpr(std::is_same_v<parameter_type, nana::charset>)
			{
				sstream_ << t.to_bytes(nana::unicode::utf8);
			}
#ifdef __cpp_char8_t
			else if constexpr(std::is_same_v<parameter_type, std::u8string> || std::is_same_v<parameter_type, char8_t> || std::is_same_v<parameter_type, std::u8string_view>)
			{
				if constexpr (std::is_same_v<std::remove_cv_t<T>, wchar_t>)
					sstream_ << static_cast<char>(t);
				else
					sstream_ << ::nana::to_string(t);
			}
#endif
			else
				sstream_<<t;

			return *this;
		}

		/// \brief Displays the message buffered in the stream.
		/// @return, the button the user clicked.
		pick_t show() const;

		/// A function object method alternative to show()
		pick_t operator()() const
		{
			return show();
		}
	private:
		std::stringstream sstream_;
		window wd_;
		std::string title_;
		button_t button_;
		icon_t icon_;
	};

	/// Simple convenience dialog to get values from the user.
	///
	/// The input value can be a boolean, string, a number, an option from a dropdown list or a date.
	class inputbox
	{
		struct abstract_content
		{
			virtual ~abstract_content() = default;

			virtual const ::std::string& label() const = 0;
			virtual window create(window, unsigned label_px) = 0;
			virtual unsigned fixed_pixels() const;
		};

	public:

	    /// Shows a checkbox for boolean input
		class boolean
			: public abstract_content
		{
			struct implement;
		public:
			boolean(::std::string label, bool initial_value);
			~boolean();

			bool value() const;
		private:
			//Implementation of abstract_content
			const ::std::string& label() const override;
			window create(window, unsigned label_px) override;
			unsigned fixed_pixels() const override;
		private:
			std::unique_ptr<implement> impl_;
		};

	    /// Integer input
		class integer
			: public abstract_content
		{
			struct implement;
		public:
			integer(::std::string label, int init_value, int begin, int last, int step);
			~integer();

			int value() const;
		private:
			//Implementation of abstract_content
			const ::std::string& label() const override;
			window create(window, unsigned label_px) override;
			unsigned fixed_pixels() const override;
		private:
			std::unique_ptr<implement> impl_;
		};

		/// Floating-point number input.
		class real
			: public abstract_content
		{
			struct implement;
		public:
			real(::std::string label, double init_value, double begin, double last, double step);
			~real();

			double value() const;
		private:
			//Implementation of abstract_content
			const ::std::string& label() const override;
			window create(window, unsigned label_px) override;
			unsigned fixed_pixels() const override;
		private:
			std::unique_ptr<implement> impl_;
		};

		/// String input or an option from a dropdown list.
		class text
			: public abstract_content
		{
			struct implement;

			text(const text&) = delete;
			text& operator=(const text&) = delete;
		public:
			text(::std::string label, ::std::string init_text = ::std::string());
			text(::std::string label, std::vector<::std::string>);

			~text();

			void tip_string(std::wstring tip);
			void tip_string(std::string tip_utf8);

			void mask_character(wchar_t ch);

			::std::string value() const;
		private:
			//Implementation of abstract_content
			const ::std::string& label() const override;
			window create(window, unsigned label_px) override;
			unsigned fixed_pixels() const override;
		private:
			std::unique_ptr<implement> impl_;
		};

		/// Date input
		class date
			: public abstract_content
		{
			struct implement;
		public:
			date(::std::string label);

			~date();

			::std::string value() const;
			int year() const;
			int month() const;	//[1, 12]
			int day() const;	//[1, 31]
		private:
			//Implementation of abstract_content
			const ::std::string& label() const override;
			window create(window, unsigned label_px) override;
			unsigned fixed_pixels() const override;
		private:
			std::unique_ptr<implement> impl_;
		};

		/// Path of a file.
		///
		/// The path requires an object of filebox. When the user clicks the `Browse` button,
		/// it invokes the filebox with proper configurations to query a filename.
		class path
			: public abstract_content
		{
			struct implement;
		public:
			path(::std::string label, const ::nana::filebox&);
			~path();

			::std::string value() const;
		private:
			//Implementation of abstract_content
			const ::std::string& label() const override;
			window create(window, unsigned label_px) override;
		private:
			std::unique_ptr<implement> impl_;
		};

		inputbox(window owner,     ///< A handle to an owner window (just a parent form or widget works)
		         ::std::string description,   ///< tells users what the purpose for the input. It can be a formatted-text.
                 ::std::string title = ::std::string()  ///< The title for the inputbox.
        );

		/// shows images at left/right side of inputbox
		void image(::nana::paint::image image,      ///< The image
		           bool is_left,      ///< true to place the image at left side, false to the right side
		           const rectangle& valid_area = {} ///< The area of the image to be displayed
        );

		/// shows images at top/bottom side of inputbox
		void image_v(::nana::paint::image,     ///< The image
		              bool is_top,    ///< `true` to place the image at top side, `false` at bottom side
		              const rectangle& valid_area = {}    ///< The area of the image to be displayed
        );

		/// Shows the inputbox and wait for the user input.
		///
		/// This method shows the inputbox without preventing the user interacts with other windows.
		template<typename ...Args>
		bool show(Args&& ... args)
		{
			std::vector<abstract_content*> contents;

			(contents.emplace_back(&args), ...);
			if (contents.empty())
				return false;

			return _m_open(contents, false);
		}

        /// Shows the inputbox and wait for the user input blocking other windows.
        ///
        /// This method blocks the execution and prevents user interaction with other
        /// windows until the inputbox is closed.
        template<typename ...Args>
		bool show_modal(Args&& ... args)
		{
			std::vector<abstract_content*> contents;

			(contents.emplace_back(&args), ...);
			if (contents.empty())
				return false;

			return _m_open(contents, true);
		}

		/// Sets a verifier to verify the user input, taking a handle to the inputbox.
		void verify(std::function<bool(window)> verifier);

		/** Sets the minimum width for the entry fields
            @param[in] pixels  new minimum width

            If not called, the default is 100 pixels
        */
		void min_width_entry_field( unsigned pixels );

	private:
		bool _m_open(std::vector<abstract_content*>&, bool modal);
	private:
		window owner_;
		::std::string description_;
		::std::string title_;
		std::function<bool(window)> verifier_;
		::nana::paint::image images_[4];
		::nana::rectangle valid_areas_[4];
        unsigned min_width_entry_field_pixels_;
	};
}//end namespace nana
#include <nana/pop_ignore_diagnostic>

#endif
