/*
 * Author: Sven Gothel <sgothel@jausoft.com> and Svenson Han Gothel
 * Copyright (c) 2022 Gothel Software e.K.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "pixel/pixel.hpp"
#include "pixel/audio.hpp"
#include "jau/utils.hpp"

#include <atomic>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_timer.h>

using namespace jau;
//
// audio
//

static std::atomic_bool audio_subsystem_init_called = false;
static std::atomic_bool audio_subsystem_init = false;

bool jau::audio::is_audio_subsystem_initialized() noexcept {
    return audio_subsystem_init;
}

bool jau::audio::init_audio_subsystem(int init_modules, int req_modules, int mix_channels, int out_channel, int out_frequency, Uint16 /*out_sample_format*/, int out_chunksize) {
    bool exp_init_called = false;
    if( !audio_subsystem_init_called.compare_exchange_strong(exp_init_called, true) ) {
        log_printf("SDL_mixer: Initialization already called: Initialized %d\n", audio_subsystem_init.load());
        return audio_subsystem_init;
    }
    if (SDL_Init(SDL_INIT_AUDIO) != 0) {
        printf("SDL: Error initializing audio: %s\n", SDL_GetError());
        return false;
    }
    if( 0 != Mix_OpenAudio(out_frequency, AUDIO_S16SYS, out_channel, out_chunksize) ) {
        log_printf("SDL_mixer: Error Mix_OpenAudio: %s\n", SDL_GetError());
        return false;
    }

    if ( ( Mix_Init(init_modules) & req_modules ) != req_modules ) {
        log_printf("SDL_mixer: Error initializing: %s\n", SDL_GetError());
        return false;
    }

    {
        log_printf("SDL_mixer: ChunkDecoder: ");
        const int max=Mix_GetNumChunkDecoders();
        for(int i=0; i<max; ++i) {
            fprintf(stderr, "%s, ", Mix_GetChunkDecoder(i));
        }
        fprintf(stderr, "\n");
    }
    {
        log_printf("SDL_mixer: MusicDecoder: ");
        const int max=Mix_GetNumMusicDecoders();
        for(int i=0; i<max; ++i) {
            fprintf(stderr, "%s, ", Mix_GetMusicDecoder(i));
        }
        fprintf(stderr, "\n");
    }
    Mix_AllocateChannels(mix_channels);
    audio_subsystem_init = true;
    log_printf("SDL_mixer: Initialized\n");
    return true;
}

void jau::audio::audio_close() {
    if( audio_subsystem_init ) {
        Mix_CloseAudio();
    }
}

static Mix_Chunk* Mix_LoadWAV2(const std::string& f) {
    const std::string g = pixel::resolve_asset(f);
    if( g.size() > 0 ) {
        return Mix_LoadWAV(g.c_str());
    } else {
        return nullptr;
    }
}
jau::audio::audio_sample_t::audio_sample_t(const std::string &fname, const bool single_play, const int volume)
: chunk(Mix_LoadWAV2(fname.c_str()), Mix_FreeChunk), channel_playing(-1), singly(single_play)
{
    if ( !chunk ) {
        log_printf("Mix_LoadWAV: Load '%s' -> '%s' (asset dir '%s'), Error: %s\n", fname.c_str(), pixel::resolve_asset(fname).c_str(), pixel::asset_dir().c_str(), SDL_GetError());
    } else {
        Mix_VolumeChunk(chunk.get(), volume);
    }
}

void jau::audio::audio_sample_t::play(int loops) {
    if ( audio_subsystem_init && nullptr != chunk.get() ) {
        if( !singly || 0 > channel_playing || ( 0 <= channel_playing && 0 == Mix_Playing(channel_playing) ) ) {
            channel_playing = Mix_PlayChannel(-1, chunk.get(), loops - 1);
        }
    }
}

void jau::audio::audio_sample_t::stop() {
    if( audio_subsystem_init && 0 <= channel_playing ) {
        Mix_HaltChannel(channel_playing);
        channel_playing = -1;
    }
}
void jau::audio::audio_sample_t::set_volume(int volume) {
    if ( audio_subsystem_init && nullptr != chunk.get() ) {
        Mix_VolumeChunk(chunk.get(), volume);
    }
}
