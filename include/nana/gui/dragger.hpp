#ifndef NANA_GUI_DRAGGER_HPP
#define NANA_GUI_DRAGGER_HPP
#include "programming_interface.hpp"

namespace nana
{
	/// \brief Helper class that allows the user to drag windows. 
    ///
    /// If a user presses the mouse on the specified window and moves the mouse, the specified window is dragged.
    /// The drag target window and trigger window can be set more than once. 
    /// See [Is it possible to make event inside event handler?](https://nanapro.codeplex.com/discussions/444121)
    /// and [How to make widget movable by mouse the best way](https://nanapro.codeplex.com/discussions/444058)

    class dragger
		: nana::noncopyable
	{
		class dragger_impl_t;
	public:
		dragger();
		~dragger();
		void target(window);
		void target(window, const rectangle& restrict_area, nana::arrange);
		void remove_target(window);
		void trigger(window);
	private:
		dragger_impl_t * impl_;
	};
}//end namespace nana
#endif
