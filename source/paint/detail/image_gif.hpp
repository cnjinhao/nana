/*
 *	GIF Format Graphics Implementation
 *	Nana C++ Library(http://www.nanapro.org)
 *	Copyright(C) 2020 Jinhao(cnjinhao@hotmail.com)
 *
 *	Distributed under the Boost Software License, Version 1.0.
 *	(See accompanying file LICENSE_1_0.txt or copy at
 *	http://www.boost.org/LICENSE_1_0.txt)
 *
 *	@file: nana/paint/detail/image_gif.hpp
 */
#ifndef NANA_PAINT_DETAIL_IMAGE_GIF_HPP
#define NANA_PAINT_DETAIL_IMAGE_GIF_HPP

#include <memory>
#include <cstring>
#include "image_pixbuf.hpp"

//Separate the libgif from the package that system provides.
#if defined(NANA_LIBGIF)
	#include <nana_extrlib/gif_lib.h>
#else
	#include <gif_lib.h>
#endif

namespace nana{	namespace paint
{
	namespace detail
	{

		class image_gif
			:public basic_image_pixbuf
		{
			struct frame_extension
			{
				std::size_t duration{ 0 };	//Duration of the frame, in milliseconds
				std::uint16_t transparency{ 0xFF00 };
			};
		public:
			~image_gif()
			{
				this->close();
			}

			void close() override
			{
				basic_image_pixbuf::close();

				if(gif_)
				{
					int error_code;
					::DGifCloseFile(gif_, &error_code);	//Not sure if a nullptr is acceptable by DGifCloseFile.
					gif_ = nullptr;
				}
			}

			bool open(const void* file_data, std::size_t bytes) override
			{
				return false;
			}

			bool open(const std::filesystem::path& p) override
			{
				this->close();

				int error_code;
				gif_ = ::DGifOpenFileName(p.string().c_str(), &error_code);
				if(nullptr == gif_)
					return false;

				::DGifSlurp(gif_);
				frame_idx_ = 0;
				_m_read_extension();


				pixbuf_.open(gif_->SWidth, gif_->SHeight);

				_m_write_rgb(gif_->SavedImages);
				return true;
			}

			bool alpha_channel() const override
			{
				return false;
			}

			std::size_t length() const override
			{
				return (gif_ ? gif_->ImageCount : 0);
			}

			std::size_t frame() const override
			{
				return frame_idx_;
			}

			std::size_t frame_duration() const override
			{
				if(frame_idx_ < extensions_.size())
					return extensions_[frame_idx_].duration;

				return 0;
			}

			bool set_frame(std::size_t pos) override
			{
				if(gif_ && (pos < gif_->ImageCount))
				{
					frame_idx_ = pos;
					_m_write_rgb(gif_->SavedImages + pos);
					return true;
				}
				return false;
			}
		private:
			void _m_write_rgb(SavedImage* img)
			{
				auto const cmap = (img->ImageDesc.ColorMap ? img->ImageDesc.ColorMap : gif_->SColorMap);
				if(nullptr == cmap)
					return;

				auto const bottom = img->ImageDesc.Top + img->ImageDesc.Height;
				auto index_ptr = img->RasterBits;

				auto transparency = extensions_[frame_idx_].transparency;

				for(auto top = img->ImageDesc.Top; top < bottom; ++top)
				{
					auto px = pixbuf_.raw_ptr(top) + img->ImageDesc.Left;

					
					for(auto const end = px + img->ImageDesc.Width; px < end; ++px)
					{
						if(*index_ptr != transparency)
						{
							auto rgb = cmap->Colors + *index_ptr;
							px->element.red = rgb->Red;
							px->element.green = rgb->Green;
							px->element.blue = rgb->Blue;
							px->element.alpha_channel = 0xFF;
						}

						++index_ptr;
					}
				}
			}

			void _m_read_extension()
			{
				for(int i = 0; i < gif_->ImageCount; ++i)
				{
					auto img = gif_->SavedImages + i;


					auto & ext = extensions_.emplace_back();

					for(int u = 0; u < img->ExtensionBlockCount; ++u)
					{
						if(GRAPHICS_EXT_FUNC_CODE /*0xF9*/ == img->ExtensionBlocks[u].Function)
						{
							if(img->ExtensionBlocks[u].ByteCount == 4)
							{
								//GIF delay specifies the number of hundredths (1/100) of a second
								int endian = 0x01;
								if(*reinterpret_cast<char*>(&endian) == 0x01)
									ext.duration = *reinterpret_cast<unsigned short*>(img->ExtensionBlocks[u].Bytes + 1) * 10;
								else
									ext.duration = ((std::size_t(img->ExtensionBlocks[u].Bytes[1]) << 8) | img->ExtensionBlocks[u].Bytes[2]) * 10 ;

								if(img->ExtensionBlocks[u].Bytes[0] & 0x1)
									ext.transparency = img->ExtensionBlocks[u].Bytes[3];
							}
						}
					}
				}
			}
		private:
			::GifFileType * gif_{ nullptr };
			std::size_t frame_idx_{ 0 };
			std::vector<frame_extension> extensions_;
		};//end class bmpfile
	}//end namespace detail
}//end namespace paint
}//end namespace nana

#endif
