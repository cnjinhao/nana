#include <nana/audio/detail/audio_stream.hpp>
#ifdef NANA_ENABLE_AUDIO

#include <nana/charset.hpp>

namespace nana{	namespace audio
{
	namespace detail
	{
		//class audio_stream
			bool audio_stream::open(const std::string& file)
			{
				fs_.open(to_osmbstr(file), std::ios::binary);
				if(fs_)
				{
					wave_spec::master_riff_chunk riff;
					fs_.read(reinterpret_cast<char*>(&riff), sizeof(riff));
					if(riff.ckID == *reinterpret_cast<const unsigned*>("RIFF") && riff.waveID == *reinterpret_cast<const unsigned*>("WAVE"))
					{
						fs_.read(reinterpret_cast<char*>(&ck_format_), sizeof(ck_format_));
						if(ck_format_.ckID == *reinterpret_cast<const unsigned*>("fmt ") && ck_format_.wFormatTag == 1)	//Only support PCM format
						{
							if (ck_format_.cksize > 16)
								fs_.seekg(ck_format_.cksize - 16, std::ios::cur);

							std::size_t cksize = _m_locate_chunck(*reinterpret_cast<const unsigned*>("data"));
							if(cksize)
							{
								pcm_data_pos_ = static_cast<std::size_t>(fs_.tellg());
								pcm_data_size_ = cksize;
								return true;
							}
						}
					}
				}
				return false;
			}

			void audio_stream::close()
			{
				fs_.close();
			}

			bool audio_stream::empty() const
			{
				return (!fs_);
			}

			const wave_spec::format_chunck & audio_stream::format() const
			{
				return ck_format_;
			}

			std::size_t audio_stream::data_length() const
			{
				return data_size_;
			}

			void audio_stream::locate()
			{
				fs_.clear();
				fs_.seekg(pcm_data_pos_, std::ios::beg);
				data_size_ = pcm_data_size_;
			}

			std::size_t audio_stream::read(void * buf, std::size_t len)
			{
				fs_.read(reinterpret_cast<char*>(buf), static_cast<std::streamsize>(len <= data_size_ ? len : data_size_));
				std::size_t read_bytes = static_cast<std::size_t>(fs_.gcount());
				data_size_ -= read_bytes;
				return read_bytes;
			}

			std::size_t audio_stream::_m_locate_chunck(unsigned ckID)
			{
				chunck ck;
				while(true)
				{
					fs_.read(reinterpret_cast<char*>(&ck), sizeof(ck));
					if(fs_.gcount() != sizeof(ck))
						break;

					if(ck.ckID == ckID)
						return ck.cksize;
					if(ck.ckID == *reinterpret_cast<const unsigned*>("data"))
						fs_.seekg(ck.cksize + (ck.cksize & 1 ? 1 : 0), std::ios::cur);
					else
						fs_.seekg(ck.cksize, std::ios::cur);
				}
				return 0;
			}
		//end class audio_stream
	}//end namespace detail

}//end namespace audio
}//end namespace nana
#endif //NANA_ENABLE_AUDIO
