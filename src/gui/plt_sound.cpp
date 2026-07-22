/*
  RPCEmu - An Acorn system emulator

  Copyright (C) 2025-2026 Andy Timmins

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

extern "C" {
#include "rpcemu.h"
}

#include <SDL.h>

#include <mutex>

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
		/* Back-pressure must reflect what is genuinely still waiting to be
		   played, i.e. SDL's own queue - not a local staging buffer that play()
		   drains completely on every call. Measuring the latter reported the
		   queue as permanently empty, so every block was pushed into SDL's
		   unbounded queue immediately; any transient burst (catch-up after a
		   HostFS stall, scheduler jitter) then inflated the backlog for good,
		   and audio latency ratcheted up and never recovered. */
		const int32_t capacity = static_cast<int32_t>(bufferlen_ * kLatencyBuffers);
		const int32_t queued = static_cast<int32_t>(SDL_GetQueuedAudioSize(device_));
		const int32_t freebytes = capacity - queued;
		return freebytes > 0 ? freebytes : 0;
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

		/* Hand the block straight to SDL. Upstream back-pressure (bufferFree,
		   consulted by sound_buffer_update before each call) bounds how far
		   ahead we queue, so no local staging buffer is needed. */
		std::lock_guard<std::mutex> lock(mutex_);
		SDL_QueueAudio(device_, buffer, length);
	}

private:
	void warnNoAudio(const char *detail)
	{
		if (!warned_) {
			warned_ = true;
			rpclog("plt_sound: %s (audio disabled)\n", detail);
		}
	}

	/* Target audio latency, expressed as a multiple of one RISC OS sound
	   buffer (~53ms each). Four buffers (~210ms) leaves comfortable headroom
	   against host scheduler jitter while keeping the delay low enough that
	   beeps and other short sounds feel prompt. */
	static constexpr uint32_t kLatencyBuffers = 4;

	uint32_t bufferlen_ = 0;
	uint32_t samplerate_ = 0;
	SDL_AudioDeviceID device_ = 0;
	bool muted_ = false;
	bool warned_ = false;
	mutable std::mutex mutex_;
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
