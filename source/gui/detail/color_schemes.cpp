#include <nana/gui/detail/color_schemes.hpp>
#include <map>

namespace nana
{
	//class color_proxy
		color_proxy::color_proxy(const color_proxy& other)
			: color_(other.color_)
		{}

		color_proxy::color_proxy(color_rgb clr)
			: color_(std::make_shared<color>(clr))
		{}

		color_proxy::color_proxy(colors clr)
			: color_(std::make_shared<color>(clr))
		{}

		color_proxy& color_proxy::operator=(const color_proxy& other)
		{
			if (this != &other)
				color_ = other.color_;
			return *this;
		}

		color_proxy& color_proxy::operator=(const ::nana::color& clr)
		{
			color_ = std::make_shared<::nana::color>(clr);
			return *this;
		}

		color_proxy& color_proxy::operator = (color_rgb clr)
		{
			color_ = std::make_shared<::nana::color>(clr);
			return *this;
		}

		color_proxy& color_proxy::operator = (colors clr)
		{
			color_ = std::make_shared<::nana::color>(clr);
			return *this;
		}

		color color_proxy::get_color() const
		{
			return *color_;
		}

		color_proxy::operator color() const
		{
			return *color_;
		}
	//end class color_proxy

	namespace detail
	{
		//class color_schemes
			struct color_schemes::implement
			{
				std::map<scheme_factory_base::factory_identifier*, std::unique_ptr<scheme>> scheme_template;
			};

			color_schemes::color_schemes()
				: impl_(new implement)
			{
			}

			color_schemes::~color_schemes()
			{
				delete impl_;
			}

			auto color_schemes::scheme_template(scheme_factory_base&& factory) -> scheme&
			{
				auto & tmpl_scheme = impl_->scheme_template[factory.get_id()];

				//Creates a scheme template if no template
				if (!tmpl_scheme)
					tmpl_scheme.reset(factory.create());

				return *tmpl_scheme.get();
			}

			widget_colors* color_schemes::create(scheme_factory_base&& factory)
			{
				return factory.create(scheme_template(std::move(factory)));
			}
		//end class color_system
	}//end namespace detail
}//end namespace nana