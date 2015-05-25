#pragma once

#include <memory>
#include <nana/gui/widgets/widget.hpp>

namespace nana {
namespace detail {

class widget_notifier_interface {
public:
	virtual widget* widget_ptr() const = 0;
	virtual void destroy() = 0;
	virtual std::wstring caption() = 0;
	virtual void caption(std::wstring text) = 0;
	
	static std::unique_ptr<widget_notifier_interface> get_notifier(widget* wdg);
private:
	widget::notifier* p;
};

} // namespace detail
} // namespace nana
