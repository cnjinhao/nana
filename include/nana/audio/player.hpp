#ifndef NANA_AUDIO_PLAYER_HPP
#define NANA_AUDIO_PLAYER_HPP
#include <nana/traits.hpp>
#include <nana/deploy.hpp>

namespace nana{	namespace audio
{       /// play an audio file in Windows WAV format
	class player
		: private nana::noncopyable
	{
		struct implementation;
	public:
		player();
		player(const nana::string& file);
		~player();

		bool open(const nana::string& file);
		void play();
		void close();
	private:
		implementation* impl_;
	};
}//end namespace audio
}//end namespace nana
#endif