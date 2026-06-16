extern "C" {
#include "rpcemu.h"
}

#include <SDL.h>

#include <algorithm>
#include <cstring>
#include <mutex>
#include <vector>

namespace {

class SdlAudioOutput {
public:
	explicit SdlAudioOutput(uint32_t bufferlen)
		: bufferlen_(bufferlen)
	{
		rpclog("plt_sound: SDL2 audio backend\n");
	}

	~SdlAudioOutput()
	{
		if (device_ != 0) {
			SDL_CloseAudioDevice(device_);
			device_ = 0;
		}
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
	}

	void setMuted(bool muted) { muted_ = muted; }
	bool isMuted() const { return muted_; }

	void changeSampleRate(uint32_t samplerate)
	{
		if (device_ != 0) {
			SDL_CloseAudioDevice(device_);
			device_ = 0;
		}

		samplerate_ = samplerate;
		queue_.clear();
		read_pos_ = 0;

		if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
			warnNoAudio(SDL_GetError());
			return;
		}

		SDL_AudioSpec want{};
		want.freq = static_cast<int>(samplerate);
		want.format = AUDIO_S16SYS;
		want.channels = 2;
		want.samples = 1024;
		want.callback = nullptr;

		SDL_AudioSpec have{};
		device_ = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
		if (device_ == 0) {
			warnNoAudio(SDL_GetError());
			return;
		}

		SDL_PauseAudioDevice(device_, 0);
		rpclog("plt_sound: opened SDL device at %dHz\n", have.freq);
	}

	int32_t bufferFree() const
	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (device_ == 0) {
			return static_cast<int32_t>(bufferlen_);
		}
		const int32_t queued = static_cast<int32_t>(queue_.size() - read_pos_);
		const int32_t capacity = static_cast<int32_t>(bufferlen_ * 8);
		return capacity - queued;
	}

	void play(uint32_t samplerate, const char *buffer, uint32_t length)
	{
		if (samplerate != samplerate_) {
			rpclog("plt_sound: changing to samplerate %uHz\n", samplerate);
			changeSampleRate(samplerate);
		}

		if (device_ == 0 || muted_ || !config.soundenabled) {
			return;
		}

		std::lock_guard<std::mutex> lock(mutex_);
		const size_t old_size = queue_.size();
		queue_.resize(old_size + length);
		memcpy(queue_.data() + old_size, buffer, length);

		uint8_t stream[4096];
		while (true) {
			const size_t available = queue_.size() - read_pos_;
			if (available == 0) {
				break;
			}

			const size_t to_copy = std::min(available, sizeof(stream));
			memcpy(stream, queue_.data() + read_pos_, to_copy);
			SDL_QueueAudio(device_, stream, static_cast<Uint32>(to_copy));
			read_pos_ += to_copy;

			if (read_pos_ > queue_.size() / 2) {
				queue_.erase(queue_.begin(),
				              queue_.begin() + static_cast<std::ptrdiff_t>(read_pos_));
				read_pos_ = 0;
			}
		}
	}

private:
	void warnNoAudio(const char *detail)
	{
		if (!warned_) {
			warned_ = true;
			rpclog("plt_sound: %s (audio disabled)\n", detail);
		}
	}

	uint32_t bufferlen_ = 0;
	uint32_t samplerate_ = 0;
	SDL_AudioDeviceID device_ = 0;
	bool muted_ = false;
	bool warned_ = false;
	mutable std::mutex mutex_;
	std::vector<uint8_t> queue_;
	size_t read_pos_ = 0;
};

SdlAudioOutput *audio_out = nullptr;

} // namespace

extern "C" void plt_sound_init(uint32_t bufferlen)
{
	audio_out = new SdlAudioOutput(bufferlen);
	if (audio_out == nullptr) {
		fatal("plt_sound_init: out of memory");
	}
}

extern "C" void plt_sound_restart(void)
{
	if (audio_out != nullptr && config.soundenabled) {
		audio_out->setMuted(false);
	}
}

extern "C" void plt_sound_pause(void)
{
	if (audio_out != nullptr) {
		audio_out->setMuted(true);
	}
}

extern "C" int32_t plt_sound_buffer_free(void)
{
	if (audio_out == nullptr) {
		return 0;
	}
	return audio_out->bufferFree();
}

extern "C" void plt_sound_buffer_play(uint32_t samplerate, const char *buffer, uint32_t length)
{
	if (audio_out == nullptr || buffer == nullptr || length == 0) {
		return;
	}
	audio_out->play(samplerate, buffer, length);
}

extern "C" void plt_sound_set_muted(int muted)
{
	if (audio_out != nullptr) {
		audio_out->setMuted(muted != 0);
	}
}

extern "C" int plt_sound_is_muted(void)
{
	if (audio_out != nullptr) {
		return audio_out->isMuted() ? 1 : 0;
	}
	return 0;
}
