#ifndef NANA_AUDIO_PLAYER_HPP
#define NANA_AUDIO_PLAYER_HPP
#include <nana/traits.hpp>
#include <nana/deploy.hpp>

namespace nana{	namespace audio
{       /// class player
        /// \brief play an audio file in PCM Windows WAV format 
        ///
        /// \include  audio_player.cpp
	class player
		: private nana::noncopyable
	{
		struct implementation;
	public:
		player();
		player(const std::string& file);
		~player();

		bool open(const std::string& file);
		void play();
		void close();
	private:
		implementation* impl_;
	};
}//end namespace audio
}//end namespace nana
#endif