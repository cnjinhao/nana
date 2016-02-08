#include <nana/audio/player.hpp>

#ifdef NANA_ENABLE_AUDIO

#include <nana/audio/detail/audio_stream.hpp>
#include <nana/audio/detail/audio_device.hpp>
#include <nana/audio/detail/buffer_preparation.hpp>
#include <nana/system/platform.hpp>

namespace nana{	namespace audio
{
	//class player
		struct player::implementation
		{
			detail::audio_stream	stream;
			detail::audio_device	dev;
		};

		player::player()
			: impl_(new implementation)
		{}

		player::player(const std::string& file)
			: impl_(new implementation)
		{
			open(file);
		}

		player::~player()
		{
			delete impl_;
		}

		bool player::open(const std::string& file)
		{
			if(impl_->stream.open(file))
			{
				const detail::wave_spec::format_chunck & ck = impl_->stream.format();
				return impl_->dev.open(ck.nChannels, ck.nSamplePerSec, ck.wBitsPerSample);
			}
			return false;
		}

		void player::play()
		{
			if(impl_->dev.empty() || impl_->stream.empty()) return;

			//Locate the PCM
			impl_->stream.locate();
			const std::size_t seconds = 5;

			detail::buffer_preparation buffer(impl_->stream, seconds);
			impl_->dev.prepare(buffer);
			detail::buffer_preparation::meta * meta;
			while((meta = buffer.read()))
			{
				impl_->dev.write(meta);
			}
			impl_->dev.wait_for_drain();
		}

		void player::close()
		{
			impl_->dev.close();
			impl_->stream.close();
		}
}//end namespace audio
}//end namespace nana

#endif //NANA_ENABLE_AUDIO