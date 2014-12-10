#ifndef NANA_PAINT_IMAGE_PROCESS_SELECTOR_HPP
#define NANA_PAINT_IMAGE_PROCESS_SELECTOR_HPP
#include "detail/image_process_provider.hpp"

namespace nana
{
	namespace paint
	{
		namespace image_process
		{           /// Configure the image processing algorithms.
			class selector
			{
			public:
				            /// Selects an image processor through a specified name.
                        /*! if users give a non-existing name for choosing an image processor, 
                             the call succeed, but no image processor would be replaced. 
                         */
				void stretch(const std::string& name);
                            /// Inserts a new user-defined image processor for stretch.
				template<typename ImageProcessor>
				void add_stretch(const std::string& name)
				{
					detail::image_process_provider & p = detail::image_process_provider::instance();
					p.add<ImageProcessor>(p.ref_stretch_tag(), name);
				}

				            /// Selects an image process through a specified name.
				void alpha_blend(const std::string& name);
				            /// Inserts a new user defined image process for alpha blend.
                template<typename ImageProcessor>
				void add_alpha_blend(const std::string& name)
				{
					detail::image_process_provider& p = detail::image_process_provider::instance();
					p.add<ImageProcessor>(p.ref_alpha_blend_tag(), name);
				}

			    	/// Selects an image processor blend through a specified name.
				void blend(const std::string& name);
                    /// Inserts a new user-defined image processor for blend.
				template<typename ImageProcessor>
				void add_blend(const std::string& name)
				{
					detail::image_process_provider& p = detail::image_process_provider::instance();
					p.add<ImageProcessor>(p.ref_blend_tag(), name);
				}

	    			/// Selects an image processor through a specified name.
				void line(const std::string& name);
                    /// Inserts a new user-defined image processor for line.
				template<typename ImageProcessor>
				void add_line(const std::string& name)
				{
					detail::image_process_provider & p = detail::image_process_provider::instance();
					p.add<ImageProcessor>(p.ref_line_tag(), name);
				}
				
				        /// blur -  	Selects an image procssor through a specified name.
				void blur(const std::string& name);
                        /// Inserts a new user-defined image process for blur.
				template<typename ImageProcessor>
				void add_blur(const std::string& name)
				{
					detail::image_process_provider & p = detail::image_process_provider::instance();
					p.add<ImageProcessor>(p.ref_blur_tag(), name);
				}
			};
		}
	}
}

#endif
