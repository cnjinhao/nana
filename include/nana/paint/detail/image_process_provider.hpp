#ifndef NANA_PAINT_IMAGE_PROCESS_PROVIDER_HPP
#define NANA_PAINT_IMAGE_PROCESS_PROVIDER_HPP
#include <nana/pat/cloneable.hpp>
#include <nana/paint/image_process_interface.hpp>
#include <string>
#include <map>

namespace nana
{
	namespace paint
	{
		namespace detail
		{
			class image_process_provider
				: nana::noncopyable
			{
				image_process_provider();

				struct stretch_tag
				{
					typedef paint::image_process::stretch_interface	interface_t;
					typedef pat::mutable_cloneable<interface_t>	cloneable_t;
					typedef	std::map<std::string, cloneable_t>		table_t;

					table_t table;
					interface_t* employee;
				}stretch_;

				struct alpha_blend_tag
				{
					typedef paint::image_process::alpha_blend_interface	interface_t;
					typedef pat::mutable_cloneable<interface_t>	cloneable_t;
					typedef std::map<std::string, cloneable_t>		table_t;

					table_t	table;
					interface_t	* employee;				
				}alpha_blend_;

				struct blend_tag
				{
					typedef paint::image_process::blend_interface	interface_t;
					typedef pat::mutable_cloneable<interface_t>	cloneable_t;
					typedef std::map<std::string, cloneable_t>		table_t;

					table_t	table;
					interface_t	* employee;
				}blend_;

				struct line_tag
				{
					typedef paint::image_process::line_interface	interface_t;
					typedef pat::mutable_cloneable<interface_t>	cloneable_t;
					typedef std::map<std::string, cloneable_t>		table_t;

					table_t	table;
					interface_t * employee;
				}line_;

				struct blur_tag
				{
					typedef paint::image_process::blur_interface	interface_t;
					typedef pat::mutable_cloneable<interface_t>	cloneable_t;
					typedef std::map<std::string, cloneable_t>		table_t;

					table_t	table;
					interface_t * employee;				
				}blur_;
			public:

				static image_process_provider & instance();

				stretch_tag & ref_stretch_tag();
				paint::image_process::stretch_interface * const * stretch() const;
				paint::image_process::stretch_interface * ref_stretch(const std::string& name) const;

				alpha_blend_tag & ref_alpha_blend_tag();
				paint::image_process::alpha_blend_interface * const * alpha_blend() const;
				paint::image_process::alpha_blend_interface * ref_alpha_blend(const std::string& name) const;

				blend_tag & ref_blend_tag();
				paint::image_process::blend_interface * const * blend() const;
				paint::image_process::blend_interface * ref_blend(const std::string& name) const;

				line_tag & ref_line_tag();
				paint::image_process::line_interface * const * line() const;
				paint::image_process::line_interface * ref_line(const std::string& name) const;

				blur_tag & ref_blur_tag();
				paint::image_process::blur_interface * const * blur() const;
				paint::image_process::blur_interface * ref_blur(const std::string& name) const;
			public:
				template<typename Tag>
				void set(Tag & tag, const std::string& name)
				{
					auto i = tag.table.find(name);
					if(i != tag.table.end())
						tag.employee = &(*(i->second));
				}

				//add
				//@brief:	The add operation is successful if the name does not exist.
				//			it does not replace the existing object by new object, because this
				//			feature is thread safe and efficiency.
				template<typename ImageProcessor, typename Tag>
				void add(Tag & tag, const std::string& name)
				{
					if(tag.table.count(name) == 0)
					{
						typedef typename Tag::cloneable_t cloneable_t;
						auto & obj = tag.table[name];
						obj = cloneable_t(ImageProcessor());
						if(nullptr == tag.employee)
							tag.employee = &(*obj);
					}
				}
			private:
				template<typename Tag>
				typename Tag::interface_t* _m_read(const Tag& tag, const std::string& name) const
				{
					auto i = tag.table.find(name);
					return (i != tag.table.end() ? &(*i->second) : tag.employee);
				}
			};
		}
	}
}//end namespace nana
#endif
