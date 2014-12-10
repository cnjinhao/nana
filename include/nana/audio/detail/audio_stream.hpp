#ifndef NANA_AUDIO_DETAIL_AUDIO_STREAM_HPP
#define NANA_AUDIO_DETAIL_AUDIO_STREAM_HPP
#include <fstream>
#include <nana/deploy.hpp>

namespace nana{	namespace audio{
	namespace detail
	{
		namespace wave_spec
		{
		#if defined(NANA_WINDOWS)
			#pragma pack(1)
				struct master_riff_chunk
				{
					unsigned long	ckID;	//"RIFF"
					unsigned long	cksize;
					unsigned long	waveID;	//"WAVE"
				};

				struct format_chunck
				{
					unsigned long	ckID;	//"fmt "
					unsigned long	cksize;
					unsigned short	wFormatTag;
					unsigned short	nChannels;
					unsigned long	nSamplePerSec;
					unsigned long	nAvgBytesPerSec;
					unsigned short	nBlockAlign;
					unsigned short	wBitsPerSample;
				};
			#pragma pack()
		#elif defined(NANA_LINUX)
			struct master_riff_chunk
			{
				unsigned long	ckID;	//"RIFF"
				unsigned long	cksize;
				unsigned long	waveID;	//"WAVE"
			}__attribute__((packed));

			struct format_chunck
			{
				unsigned long	ckID;	//"fmt "
				unsigned long	cksize;
				unsigned short	wFormatTag;
				unsigned short	nChannels;
				unsigned long	nSamplePerSec;
				unsigned long	nAvgBytesPerSec;
				unsigned short	nBlockAlign;
				unsigned short	wBitsPerSample;
			}__attribute__((packed));
		#endif
		}

		class audio_stream
		{
			struct chunck
			{
				unsigned long ckID;
				unsigned long cksize;
			};
		public:
			bool open(const nana::string& file);
			void close();
			bool empty() const;
			const wave_spec::format_chunck & format() const;
			std::size_t data_length() const;
			void locate();
			std::size_t read(void * buf, std::size_t len);
		private:
			std::size_t _m_locate_chunck(unsigned ckID);
		private:
			std::ifstream fs_;
			wave_spec::format_chunck ck_format_;
			std::size_t pcm_data_pos_;
			std::size_t pcm_data_size_;
			std::size_t data_size_;
		}; //end class audio_stream
	}
}//end namespace audio
}//end namespace nana
#endif
