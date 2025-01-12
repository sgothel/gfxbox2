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
#include "utils.hpp"
#include "maze.hpp"
#include <jau/utils.hpp>
#include "game.hpp"

#include <limits>

#include <cstdio>
#include <pixel/audio.hpp>
#include <pixel/pixel.hpp>
#include <pixel/pixel2f.hpp>

//
// globals across modules 'game.hpp'
//

static int start_level = 1;
static int current_level = start_level;
int get_current_level() noexcept { return current_level; }

std::unique_ptr<maze_t> global_maze;
std::shared_ptr<global_tex_t> global_tex;
std::vector<ghost_ref> ghosts_;
pixel::texture_ref pacman_maze_tex;
pixel::texture_ref pacman_left2_tex;
pacman_ref pacman;
ghost_ref blinky;

std::vector<ghost_ref>& ghosts() noexcept { return ghosts_; }

ghost_ref ghost(const ghost_t::personality_t id) noexcept {
    const int idx = ghost_t::number(id);
    if( 0 <= idx && (size_t)idx < ghosts_.size() ) {
        return ghosts_[ ghost_t::number(id)];
    } else {
        return nullptr;
    }
}
std::vector<jau::audio::audio_sample_ref> audio_samples;

static bool original_pacman_behavior = true;
bool use_original_pacman_behavior() noexcept { return original_pacman_behavior; }

static bool decision_one_field_ahead = true;
bool use_decision_one_field_ahead() noexcept {
    return decision_one_field_ahead;
}
static bool manhatten_distance_enabled = false;
bool use_manhatten_distance() noexcept { return manhatten_distance_enabled; }

static bool enable_debug_gfx = false;
bool show_debug_gfx() noexcept { return enable_debug_gfx; }

static bool enable_log_fps = false;
static bool enable_log_moves = false;
static bool enable_log_modes = false;
bool log_fps() noexcept { return enable_log_fps; }
bool log_moves() noexcept { return enable_log_moves; }
bool log_modes() noexcept { return enable_log_modes; }

//
// score_t
//

score_t tile_to_score(const tile_t tile) noexcept {
    switch(tile) {
        case tile_t::PELLET: return score_t::PELLET;
        case tile_t::PELLET_POWER: return score_t::PELLET_POWER;
        case tile_t::CHERRY: return score_t::CHERRY;
        case tile_t::STRAWBERRY: return score_t::STRAWBERRY;
        case tile_t::PEACH: return score_t::PEACH;
        case tile_t::APPLE: return score_t::APPLE;
        case tile_t::MELON: return score_t::MELON;
        case tile_t::GALAXIAN: return score_t::GALAXIAN;
        case tile_t::BELL: return score_t::BELL;
        case tile_t::KEY: return score_t::KEY;
        default: return score_t::NONE;
    }
}

//
// level_spec_t
//

static ghost_wave_vec_t ghost_waves_1 = { { 7000, 20000 }, { 7000, 20000 }, { 5000,   20000 }, { 5000, std::numeric_limits<int>::max() }, { 0, std::numeric_limits<int>::max() } };
static ghost_wave_vec_t ghost_waves_2 = { { 7000, 20000 }, { 7000, 20000 }, { 5000, 1033000 }, {   17, std::numeric_limits<int>::max() }, { 0, std::numeric_limits<int>::max() } };
static ghost_wave_vec_t ghost_waves_5 = { { 5000, 20000 }, { 5000, 20000 }, { 5000, 1037000 }, {   17, std::numeric_limits<int>::max() }, { 0, std::numeric_limits<int>::max() } };

static ghost_pellet_counter_limit_t pellet_counter_limit_l1 = { 0, 0, 30, 60 };
static ghost_pellet_counter_limit_t pellet_counter_limit_l2 = { 0, 0, 0, 50 };
static ghost_pellet_counter_limit_t pellet_counter_limit_l3 = { 0, 0, 0, 0 };

ghost_pellet_counter_limit_t global_ghost_pellet_counter_limit = { 0, 7, 17, 32 };

static std::vector<game_level_spec_t> level_spec_array = {
    //                                                                                                  Elroy_1,   Elroy_2
    //                                                                                                        |          |
    //                                                                            Fright Milliseconds         |          |
    //                                                                                              |         |          |
    //        Ghost                                                               Fright Speed      |         |          |
    //        Ghost                                                       Tunnel Speed       |      |         |          |
    //        Ghost                                                Normal Speed      |       |      |         |          |
    //                                                                        |      |       |      |         |          |
    //      Pac-Man                                 Powered Dots Speed        |      |       |      |         |          |
    //      Pac-Man                               Powered Speed      |        |      |       |      |         |          |
    //                                                        |      |        |      |       |      |         |          |
    //      Pac-Man                  Normal Dots Speed        |      |        |      |       |      |         |          |
    //      Pac-Man                Normal Speed      |        |      |        |      |       |      |         |          |
    //                                        |      |        |      |        |      |       |      |         |          |
    //                     Bonus Points       |      |        |      |        |      |       |      |         |          |
    //                                |       |      |        |      |        |      |       |      |         |          |
    /*  1 */ { tile_t::CHERRY,      100,   0.80f, 0.71f,   0.90f, 0.79f,   0.75f, 0.40f,  0.50f, 6000, 5,    20, 0.80f, 10, 0.85f, ghost_waves_1, pellet_counter_limit_l1, 4000 },
    /*  2 */ { tile_t::STRAWBERRY,  300,   0.90f, 0.79f,   0.95f, 0.83f,   0.85f, 0.45f,  0.55f, 5000, 5,    30, 0.90f, 15, 0.95f, ghost_waves_2, pellet_counter_limit_l2, 4000 },
    /*  3 */ { tile_t::PEACH,       500,   0.90f, 0.79f,   0.95f, 0.83f,   0.85f, 0.45f,  0.55f, 4000, 5,    40, 0.90f, 20, 0.95f, ghost_waves_2, pellet_counter_limit_l3, 4000 },
    /*  4 */ { tile_t::PEACH,       500,   0.90f, 0.79f,   0.95f, 0.83f,   0.85f, 0.45f,  0.55f, 3000, 5,    40, 0.90f, 20, 0.95f, ghost_waves_2, pellet_counter_limit_l3, 4000 },
    /*  5 */ { tile_t::APPLE,       700,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 2000, 5,    40, 1.00f, 20, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /*  6 */ { tile_t::APPLE,       700,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 5000, 5,    50, 1.00f, 25, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /*  7 */ { tile_t::MELON,      1000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 2000, 5,    50, 1.00f, 25, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /*  8 */ { tile_t::MELON,      1000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 2000, 5,    50, 1.00f, 25, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /*  9 */ { tile_t::GALAXIAN,   2000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,    60, 1.00f, 30, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 10 */ { tile_t::GALAXIAN,   2000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 5000, 5,    60, 1.00f, 30, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 11 */ { tile_t::BELL,       3000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 2000, 5,    60, 1.00f, 30, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 12 */ { tile_t::BELL,       3000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,    80, 1.00f, 40, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 13 */ { tile_t::KEY,        5000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,    80, 1.00f, 40, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 14 */ { tile_t::KEY,        5000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 3000, 5,    80, 1.00f, 40, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 15 */ { tile_t::KEY,        5000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,   100, 1.00f, 50, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 16 */ { tile_t::KEY,        5000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,   100, 1.00f, 50, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 17 */ { tile_t::KEY,        5000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,   100, 1.00f, 50, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 18 */ { tile_t::KEY,        5000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,   100, 1.00f, 50, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 19 */ { tile_t::KEY,        5000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,   120, 1.00f, 60, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 20 */ { tile_t::KEY,        5000,   1.00f, 0.87f,   1.00f, 0.87f,   0.95f, 0.50f,  0.60f, 1000, 3,   120, 1.00f, 60, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 },
    /* 21 */ { tile_t::KEY,        5000,   0.90f, 0.79f,   0.90f, 0.79f,   0.95f, 0.50f,  0.60f, 1000, 3,   120, 1.00f, 60, 1.05f, ghost_waves_5, pellet_counter_limit_l3, 3000 }
};

static constexpr int level_to_idx(const int level) noexcept {
    return 1 <= level && (size_t)level <= level_spec_array.size() ? level-1 : (int)level_spec_array.size()-1;
}

const game_level_spec_t& game_level_spec(const int level) noexcept {
    return level_spec_array[ level_to_idx( level ) ];
}

const game_level_spec_t& game_level_spec() noexcept {
    return level_spec_array[ level_to_idx( get_current_level() ) ];
}

const ghost_wave_t& get_ghost_wave(const int level, const int phase_idx) noexcept {
    const ghost_wave_vec_t& waves = game_level_spec(level).ghost_waves;
    const int idx = 0 <= phase_idx && (size_t)phase_idx < waves.size() ? phase_idx : (int)waves.size()-1;
    return waves[idx];
}
const ghost_wave_t& get_ghost_wave(const int phase_idx) noexcept {
    return get_ghost_wave( get_current_level(), phase_idx );
}

//
// global_tex_t
//

int global_tex_t::tile_to_texidx(const tile_t tile) const noexcept {
    if( tile_t::PELLET <= tile && tile <= tile_t::KEY ) {
        const int tile_i = static_cast<int>(tile);
        const int pellet_i = static_cast<int>(tile_t::PELLET);
        const int idx = tile_i - pellet_i;
        if( 0 <= idx && (size_t)idx < textures.size() ) {
            return idx;
        }
    }
    return -1;
}

int global_tex_t::validate_texidx(const int idx) const noexcept {
    if( 0 <= idx && (size_t)idx < textures.size() ) {
        return idx;
    }
    return -1;
}


global_tex_t::global_tex_t() noexcept
: all_images_( std::make_shared<pixel::texture_t>("pacman/tiles_all.png") ),
  atex_pellet_power( "PP", 0.250f, all_images_, 0, 0, 14, 14, { { 1*14, 0 }, { -1, -1} })
{
    add_sub_textures(textures, all_images_, 0, 0, 14, 14, {
            {  0*14, 0 }, {  1*14, 0 }, {  2*14, 0 }, {  3*14, 0 }, {  4*14, 0 }, {  5*14, 0 }, {  6*14, 0 },
            {  7*14, 0 }, {  8*14, 0 }, {  9*14, 0 }, { 10*14, 0 }, { 11*14, 0 }, { 12*14, 0 }, { 13*14, 0 },  } );
}

void global_tex_t::destroy() noexcept {
    textures.clear();
    all_images_ = nullptr;
}

pixel::texture_ref global_tex_t::texture(const int idx) noexcept {
    const int idx2 = validate_texidx(idx);
    return 0 <= idx2 ? textures[idx2] : nullptr;
}
pixel::texture_ref global_tex_t::texture(const int idx) const noexcept {
    const int idx2 = validate_texidx(idx);
    return 0 <= idx2 ? textures[idx2] : nullptr;
}

pixel::texture_ref global_tex_t::texture(const tile_t tile) noexcept {
    const int idx = tile_to_texidx(tile);
    return 0 <= idx ? textures[idx] : nullptr;
}
pixel::texture_ref global_tex_t::texture(const tile_t tile) const noexcept {
    const int idx = tile_to_texidx(tile);
    return 0 <= idx ? textures[idx] : nullptr;
}

void global_tex_t::draw_tile(const tile_t tile, const float x, const float y) noexcept {
    if( tile_t::PELLET_POWER == tile ) {
        atex_pellet_power.draw(x+global_maze->tile_dx(), y+global_maze->tile_dy());
    } else {
        pixel::texture_ref tex = texture(tile);
        if( nullptr != tex ) {
            tex->draw(x+global_maze->tile_dx(), y+global_maze->tile_dy());
        }
    }
}

std::string global_tex_t::toString() const {
    return "tiletex[count "+std::to_string(textures.size())+"]";
}

//
// main
//

static std::string get_usage(const std::string& exename) noexcept {
    // TODO: Keep in sync with README.md
    return "Usage: "+exename+" [-2p] [-audio] [-pixqual <int>] [-no_vsync] [-fps <int>] [-speed <int>] [-wwidth <int>] [-wheight <int>] "+
              "[-show_fps] [-show_modes] [-show_moves] [-show_targets] [-show_debug_gfx] [-show_all] "+
              "[-no_ghosts] [-invincible] [-bugfix] [-decision_on_spot] [-dist_manhatten] [-level <int>] [-record <basename-of-bmp-files>]";
}

//
// FIXME: Consider moving game state types and fields into its own class
//
enum class game_mode_t {
    NEXT_LEVEL,
    START,
    GAME,
    PAUSE
};
static std::string to_string(game_mode_t m) noexcept {
    switch( m ) {
        case game_mode_t::NEXT_LEVEL:
            return "next_level";
        case game_mode_t::START:
            return "start";
        case game_mode_t::GAME:
            return "game";
        case game_mode_t::PAUSE:
            return "pause";
        default:
            return "unknown";
    }
}
enum class game_mode_duration_t : int {
    LEVEL_START_SOUND = 4000,
    LEVEL_START       = 3000,
    START             = 2000
};
static constexpr int number(const game_mode_duration_t item) noexcept {
    return static_cast<int>(item);
}
static int game_mode_ms_left = -1;
static game_mode_t game_mode = game_mode_t::PAUSE;
static game_mode_t game_mode_last = game_mode_t::PAUSE;

static void set_game_mode(const game_mode_t m, const int caller) noexcept {
    const game_mode_t old_mode = game_mode;
    const int old_level = current_level;
    switch( m ) {
        case game_mode_t::NEXT_LEVEL:
            ++current_level;
            global_maze->reset();
            pacman->set_mode( pacman_t::mode_t::LEVEL_SETUP );
            game_mode = game_mode_t::START;
            if( audio_samples[ number( audio_clip_t::INTRO ) ]->is_valid() ) {
                audio_samples[ number( audio_clip_t::INTRO ) ]->play();
                game_mode_ms_left = number( game_mode_duration_t::LEVEL_START_SOUND );
            } else {
                game_mode_ms_left = number( game_mode_duration_t::LEVEL_START );
            }
            break;
        case game_mode_t::START:
            pacman->set_mode( pacman_t::mode_t::LEVEL_SETUP );
            game_mode = game_mode_t::START;
            game_mode_ms_left = number( game_mode_duration_t::START );
            break;
        case game_mode_t::GAME:
            if( game_mode_t::START == old_mode ) {
                pacman->set_mode( pacman_t::mode_t::START );
            }
            [[fallthrough]];
        case game_mode_t::PAUSE:
            pacman->stop_audio_loops();
            [[fallthrough]];
        default:
            game_mode = m;
            game_mode_ms_left = -1;
            break;
    }
    game_mode_last = old_mode;
    jau::log_printf("game set_mode(%d): %s -> %s [%d ms], level %d -> %d\n",
            caller, to_string(old_mode).c_str(), to_string(game_mode).c_str(), game_mode_ms_left, old_level, current_level);
}

static bool show_targets = false;
static pixel::input_event_t event;
static std::string record_bmpseq_basename;

static bool load_samples() {
    audio_samples.clear();
    if( jau::audio::is_audio_subsystem_initialized() ) {
        audio_samples.push_back( std::make_shared<jau::audio::audio_sample_t>("pacman/intro.ogg") );
        audio_samples.push_back( std::make_shared<jau::audio::audio_sample_t>("pacman/munch.wav") );
        audio_samples.push_back( std::make_shared<jau::audio::audio_sample_t>("pacman/eatfruit.ogg") );
        audio_samples.push_back( std::make_shared<jau::audio::audio_sample_t>("pacman/eatghost.ogg", false /* single_play */) );
        audio_samples.push_back( std::make_shared<jau::audio::audio_sample_t>("pacman/death.ogg") );
        // audio_samples.push_back( std::make_shared<audio_sample_t>("pacman/extrapac.ogg") );
        // audio_samples.push_back( std::make_shared<audio_sample_t>("pacman/intermission.ogg") );
        return true;
    } else {
        for(int i=0; i <= number( audio_clip_t::DEATH ); ++i) {
            audio_samples.push_back( std::make_shared<jau::audio::audio_sample_t>() );
        }
        return false;
    }
}

static void shutdown() {
    // exit
    audio_samples.clear();
    jau::audio::audio_close();

    ghosts_.clear();
    pacman->destroy();
    pacman_left2_tex = nullptr;
    global_tex->destroy();
    pacman_maze_tex = nullptr;
}

#if defined(__EMSCRIPTEN__)
    static void init_audio() {
        jau::audio::init_audio_subsystem(MIX_INIT_OGG, MIX_INIT_OGG);
        load_samples();
    }
    extern "C" {
        EMSCRIPTEN_KEEPALIVE void start_audio() noexcept { init_audio(); }
        EMSCRIPTEN_KEEPALIVE void set_debug_gfx(bool v) noexcept { enable_debug_gfx = v; }
        EMSCRIPTEN_KEEPALIVE void set_showtarget(bool v) noexcept { show_targets = v; }
        EMSCRIPTEN_KEEPALIVE void set_human_blinky(bool v) noexcept { blinky->set_manual_control(v); }
    }
#endif

void mainloop() {
    static uint64_t frame_count_total = 0;
    static uint64_t snap_count = 0;
    static uint64_t t_last = jau::getElapsedMillisecond(); // [ms]
    static bool animating = true;
    bool do_snapshot = false;
    bool set_dir_1 = false;
    bool set_dir_2 = false;
    direction_t pacman_dir = pacman->direction();
    direction_t blinky_dir = direction_t::LEFT;

    while( pixel::handle_one_event(event) ) {
        if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_CLOSE_REQ ) ) {
            printf("Exit Application\n");
            shutdown();
            #if defined(__EMSCRIPTEN__)
                emscripten_cancel_main_loop();
            #else
                exit(0);
            #endif
        } else if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_RESIZED ) ) {
            pixel::cart_coord.set_fitting(0.0f, 0.0f, (float)global_maze->px_width(), (float)global_maze->px_height());
            // pixel::cart_coord.set_fitting(0.0f, (float)global_maze->px_height(), (float)global_maze->px_width(), 0.0f);
        } else if( event.released_and_clr(pixel::input_event_type_t::RESET) ) {
            current_level = start_level - 1;
            pacman->reset_score();
            set_game_mode(game_mode_t::NEXT_LEVEL, 15);
        } else if( event.released_and_clr(pixel::input_event_type_t::F12) ) {
            do_snapshot = true;
        } else if( event.released_and_clr(pixel::input_event_type_t::F9) ) {
            enable_debug_gfx = !enable_debug_gfx;
        }
        if( event.paused() ) {
            if( animating ) {
                set_game_mode( game_mode_t::PAUSE, 14 );
            }
            animating = false;
        } else {
            if( !animating ) {
                set_game_mode( game_mode_last, 13 );
                t_last = jau::getElapsedMillisecond(); // [ms]
            }
            animating = true;
        }
    }
    const uint64_t t1 = animating ? jau::getElapsedMillisecond() : t_last; // [ms]
    const float dt = (float)(t1 - t_last) / 1000.0f; // [s]
    //const float avg_fd = pixel::gpu_avg_framedur();
    t_last = t1;

    if(animating) {
        set_dir_1 = false;
        set_dir_2 = false;

        // action
        if( event.has_any_p1() ) {
            if( event.pressed(pixel::input_event_type_t::P1_UP) ) {
                pacman_dir = direction_t::UP;
                set_dir_1 = true;
            } else if( event.pressed(pixel::input_event_type_t::P1_DOWN) ){
                pacman_dir = direction_t::DOWN;
                set_dir_1 = true;
            } else if( event.pressed(pixel::input_event_type_t::P1_LEFT) ){
                pacman_dir = direction_t::LEFT;
                set_dir_1 = true;
            } else if( event.pressed(pixel::input_event_type_t::P1_RIGHT) ){
                pacman_dir = direction_t::RIGHT;
                set_dir_1 = true;
            }
        }
        if( event.has_any_p2() ) {
            if( event.pressed(pixel::input_event_type_t::P2_UP) ) {
                blinky_dir = direction_t::UP;
                set_dir_2 = true;
            } else if( event.pressed(pixel::input_event_type_t::P2_DOWN) ){
                blinky_dir = direction_t::DOWN;
                set_dir_2 = true;
            } else if( event.pressed(pixel::input_event_type_t::P2_LEFT) ){
                blinky_dir = direction_t::LEFT;
                set_dir_2 = true;
            } else if( event.pressed(pixel::input_event_type_t::P2_RIGHT) ){
                blinky_dir = direction_t::RIGHT;
                set_dir_2 = true;
            }
        }
    }

    pixel::clear_pixel_fb(0, 0, 0, 255);

    // game
    {
        bool game_active;

        if( 0 < game_mode_ms_left ) {
            game_mode_ms_left = std::max( 0, game_mode_ms_left - jau::round_to_int(1000.0f*pixel::expected_framedur()) );
        }

        switch( game_mode ) {
            case game_mode_t::START:
                if( 0 == game_mode_ms_left ) {
                    set_game_mode( game_mode_t::GAME, 20 );
                    game_active = true;
                } else {
                    game_active = false;
                }
                break;
            case game_mode_t::PAUSE:
                game_active = false;
                break;
            case game_mode_t::GAME:
                if( 0 == global_maze->count( tile_t::PELLET ) && 0 == global_maze->count( tile_t::PELLET_POWER ) ) {
                    set_game_mode(game_mode_t::NEXT_LEVEL, 21);
                }
            [[fallthrough]];
            default:
                game_active = true;
                break;
        }

        if( game_active ) {
            if( set_dir_1 ) {
                pacman->set_dir(pacman_dir);
            }
            if( set_dir_2 && nullptr != blinky ) {
                blinky->set_dir(blinky_dir);
            }

            global_tex->tick(dt);
            ghost_t::global_tick(dt);
            if( !pacman->tick(dt) ) {
                // pacman caught and died .. post dead animation
                set_game_mode( game_mode_t::START, 22 );
            }
        }

        if( show_debug_gfx() ) {
            {
                // Red Zones + Tunnel
                const box_t& red_zone1 = global_maze->red_zone1_box();
                const box_t& red_zone2 = global_maze->red_zone2_box();
                const box_t& tunnel1 = global_maze->tunnel1_box();
                const box_t& tunnel2 = global_maze->tunnel2_box();

                pixel::set_pixel_color(255, 96, 96, 100);
                pixel::draw_box(true,
                                global_maze->x_to_px(red_zone1.x_f()), global_maze->y_to_px(red_zone1.y_f()),
                                global_maze->dx_to_px(red_zone1.width_f()), global_maze->dy_to_px(red_zone1.height_f()));
                pixel::draw_box(true,
                                global_maze->x_to_px(red_zone2.x_f()), global_maze->y_to_px(red_zone2.y_f()),
                                global_maze->dx_to_px(red_zone2.width_f()), global_maze->dy_to_px(red_zone2.height_f()));

                pixel::set_pixel_color(96, 96, 255, 100);
                pixel::draw_box(true,
                                global_maze->x_to_px(tunnel1.x_f()), global_maze->y_to_px(tunnel1.y_f()),
                                global_maze->dx_to_px(tunnel1.width_f()), global_maze->dy_to_px(tunnel1.height_f()));
                pixel::draw_box(true,
                                global_maze->x_to_px(tunnel2.x_f()), global_maze->y_to_px(tunnel2.y_f()),
                                global_maze->dx_to_px(tunnel2.width_f()), global_maze->dy_to_px(tunnel2.height_f()));
            }
            {
                // Grey Grid
                pixel::set_pixel_color(150, 150, 150, 255);
                // Horizontal lines
                for(int y = global_maze->height(); y>=0; --y) {
                    pixel::draw_line(global_maze->x_to_px(0.0f),                        global_maze->y_to_px((float)y),
                                     global_maze->x_to_px((float)global_maze->width()), global_maze->y_to_px((float)y));
                }
                // Vertical lines
                for(int x = global_maze->width(); x>=0; --x) {
                    pixel::draw_line(global_maze->x_to_px((float)x), global_maze->y_to_px(0.0f),
                                     global_maze->x_to_px((float)x), global_maze->y_to_px((float)global_maze->height()));
                }
            }
            {
                // Filled check-boxes at 0/0 and each scatter target tile
                acoord_t blinky_top_right = global_maze->top_right_scatter();
                acoord_t pinky_top_left = global_maze->top_left_scatter();
                acoord_t inky_bottom_right = global_maze->bottom_right_scatter();
                acoord_t clyde_bottom_left = global_maze->bottom_left_scatter();

                pixel::set_pixel_color(pacman_t::rgb_color[0], pacman_t::rgb_color[1], pacman_t::rgb_color[2], 255);
                pixel::draw_box(true, global_maze->x_to_px(0.0f), global_maze->y_to_px(0.0f),
                                global_maze->dx_to_px(1), global_maze->dy_to_px(1));

                pixel::set_pixel_color(
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::BLINKY ) ][0],
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::BLINKY ) ][1],
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::BLINKY ) ][2], 255);
                pixel::draw_box(true, global_maze->x_to_px((float)blinky_top_right.x_i()),
                                global_maze->y_to_px((float)blinky_top_right.y_i()),
                                global_maze->dx_to_px(1), global_maze->dy_to_px(1));

                pixel::set_pixel_color(
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::PINKY ) ][0],
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::PINKY ) ][1],
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::PINKY ) ][2], 255);
                pixel::draw_box(true, global_maze->x_to_px((float)pinky_top_left.x_i()),
                                global_maze->y_to_px((float)pinky_top_left.y_i()),
                                global_maze->dx_to_px(1), global_maze->dy_to_px(1));

                pixel::set_pixel_color(
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::INKY ) ][0],
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::INKY ) ][1],
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::INKY ) ][2], 255);
                pixel::draw_box(true, global_maze->x_to_px((float)inky_bottom_right.x_i()),
                                global_maze->y_to_px((float)inky_bottom_right.y_i()),
                                global_maze->dx_to_px(1), global_maze->dy_to_px(1));

                pixel::set_pixel_color(
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::CLYDE ) ][0],
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::CLYDE ) ][1],
                        ghost_t::rgb_color[ ghost_t::number( ghost_t::personality_t::CLYDE ) ][2], 255);
                pixel::draw_box(true, global_maze->x_to_px((float)clyde_bottom_left.x_i()),
                                global_maze->y_to_px((float)clyde_bottom_left.y_i()),
                                global_maze->dx_to_px(1), global_maze->dy_to_px(1));
            }
        }

        pacman_maze_tex->draw((float)global_maze->x_to_px(0), (float)global_maze->y_to_px(0));

        global_maze->draw( [](float x, float y, tile_t tile) {
            global_tex->draw_tile(tile, global_maze->x_to_px(x), global_maze->y_to_px(y));
        });

        pacman->draw();

        ghost_t::global_draw();

        if( show_targets ) {
            for(const ghost_ref& ghost : ghosts()) {
                if( ghost->is_scattering_or_chasing() ) {
                    const acoord_t& p1 = ghost->position();
                    const acoord_t& p2 = ghost->target();
                    pixel::set_pixel_color(
                            ghost_t::rgb_color[ ghost_t::number( ghost->id() ) ][0],
                            ghost_t::rgb_color[ ghost_t::number( ghost->id() ) ][1],
                            ghost_t::rgb_color[ ghost_t::number( ghost->id() ) ][2], 255);
                    pixel::draw_line(global_maze->x_to_px(p1.x_f()), global_maze->y_to_px(p1.y_f()),
                                     global_maze->x_to_px(p2.x_f()), global_maze->y_to_px(p2.y_f()));
                }
            }
        }

        // top line: title
        std::string top_line_text = jau::to_string("HIGH SCORE (fps %4.2f)", pixel::gpu_avg_fps());
        draw_text(top_line_text, 255, 255, 255, [&](const pixel::texture_t& /*tex*/, float &x, float&y, float tw, float /*th*/) {
            x = ( global_maze->x_to_px( (float)global_maze->width()) - tw ) / 2.0f;
            y = global_maze->y_to_px(0.0f);
        });

        // 2nd line - center: score
        draw_text(std::to_string( pacman->score() ), 255, 255, 255, [&](const pixel::texture_t& /*tex*/, float &x, float&y, float tw, float /*th*/) {
            x = ( global_maze->x_to_px( (float)global_maze->width()) - tw ) / 2.0f;
            y = global_maze->y_to_px(1.0f);
        });

        if( show_debug_gfx() ) {
            // 2nd line - right: tiles
            draw_text(std::to_string(global_maze->count(tile_t::PELLET))+" / "+std::to_string(global_maze->max(tile_t::PELLET)),
                      255, 255, 255, [&](const pixel::texture_t& /*tex*/, float &x, float&y, float tw, float /*th*/) {
                x = (float)global_maze->px_width() - tw;
                y = global_maze->y_to_px(1.0f);
            });
        }

        // optional text
        if( game_mode_t::START == game_mode ) {
            const box_t& msg_box = global_maze->message_box();
            draw_text("READY!",
                      pacman_t::rgb_color[0], pacman_t::rgb_color[1], pacman_t::rgb_color[2],
                      [&](const pixel::texture_t& /*tex*/, float &x, float&y, float tw, float /*th*/) {
                x = global_maze->x_to_px(msg_box.center_x()) - tw/2.0f;
                y = global_maze->y_to_px((float)msg_box.y());
            });
        }

        // bottom line: level
        {
            const float y = 34.0f;
            float x = 24.0f;

            for(int i=1; i <= get_current_level(); ++i, x-=2) {
                const tile_t f = game_level_spec(i).symbol;
                pixel::texture_ref f_tex = global_tex->texture(f);
                if( nullptr != f_tex ) {
                    const float dx = ( 16.0f - (float)f_tex->width ) / 2.0f / 16.0f;
                    const float dy = ( 16.0f - (float)f_tex->height + 1.0f ) / 16.0f; // FIXME: funny adjustment?
                    f_tex->draw(global_maze->x_to_px(x+dx), global_maze->y_to_px(y+dy));
                    // jau::log_printf("XX1 level %d: %s, %.2f / %.2f + %.2f / %.2f = %.2f / %.2f\n", i, to_string(f).c_str(), x, y, dx, dy, x+dx, y+dy);
                }
            }
        }
        // bottom line: lives left
        if( nullptr != pacman_left2_tex ) {
            const float dx = ( 16.0f - (float)pacman_left2_tex->width ) / 2.0f / 16.0f;
            const float dy = ( 16.0f - (float)pacman_left2_tex->height + 1.0f ) / 16.0f; // FIXME: funny adjustment?
            const float y = 34.0f;
            float x = 2.0f;
            for(int i=0; i < 2; ++i, x+=2) {
                pacman_left2_tex->draw(global_maze->x_to_px(x+dx), global_maze->y_to_px(y+dy));
                // jau::log_printf("XX2 %d: %.2f / %.2f + %.2f / %.2f = %.2f / %.2f\n", i, x, y, dx, dy, x+dx, y+dy);
            }
        }
    } // game

    // swap
    pixel::swap_pixel_fb();
    if( record_bmpseq_basename.size() > 0 ) {
        std::string snap_fname(128, '\0');
        const int written = std::snprintf(&snap_fname[0], snap_fname.size(), "%s-%7.7" PRIu64 ".bmp", record_bmpseq_basename.c_str(), frame_count_total);
        snap_fname.resize(written);
        pixel::save_snapshot(snap_fname);
    }
    if( do_snapshot ) {
        std::string snap_fname(128, '\0');
        const int written = std::snprintf(&snap_fname[0], snap_fname.size(), "pacman-%7.7" PRIu64 ".bmp", snap_count++);
        snap_fname.resize(written);
        pixel::save_snapshot(snap_fname);
    }
    ++frame_count_total;
}

int main(int argc, char *argv[])
{
    bool enable_vsync = true;
    int forced_fps = -1;
    float fields_per_sec_total=10;
    int win_width = 640, win_height = 720;
    bool disable_all_ghosts = false;
    bool invincible = false;
    bool use_audio = true;
    int pixel_filter_quality = 0;
    bool human_blinky = false;
    {
        for(int i=1; i<argc; ++i) {
            if( 0 == strcmp("-2p", argv[i]) ) {
                human_blinky = true;
            } else if( 0 == strcmp("-noaudio", argv[i]) ) {
                use_audio = false;
            } else if( 0 == strcmp("-pixqual", argv[i]) && i+1<argc) {
                pixel_filter_quality = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-no_vsync", argv[i]) ) {
                enable_vsync = false;
            } else if( 0 == strcmp("-fps", argv[i]) && i+1<argc) {
                forced_fps = atoi(argv[i+1]);
                enable_vsync = false;
                ++i;
            } else if( 0 == strcmp("-speed", argv[i]) && i+1<argc) {
                fields_per_sec_total = (float)atof(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-wwidth", argv[i]) && i+1<argc) {
                win_width = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-wheight", argv[i]) && i+1<argc) {
                win_height = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-show_fps", argv[i]) ) {
                enable_log_fps = true;
            } else if( 0 == strcmp("-show_modes", argv[i]) ) {
                enable_log_modes = true;
            } else if( 0 == strcmp("-show_moves", argv[i]) ) {
                enable_log_moves = true;
            } else if( 0 == strcmp("-show_targets", argv[i]) ) {
                show_targets = true;
            } else if( 0 == strcmp("-show_debug_gfx", argv[i]) ) {
                enable_debug_gfx = true;
            } else if( 0 == strcmp("-show_all", argv[i]) ) {
                enable_log_fps = true;
                enable_log_moves = true;
                enable_log_modes = true;
                show_targets = true;
                enable_debug_gfx = true;
            } else if( 0 == strcmp("-no_ghosts", argv[i]) ) {
                disable_all_ghosts = true;
            } else if( 0 == strcmp("-invincible", argv[i]) ) {
                invincible = true;
            } else if( 0 == strcmp("-bugfix", argv[i]) ) {
                original_pacman_behavior = false;
            } else if( 0 == strcmp("-decision_on_spot", argv[i]) ) {
                decision_one_field_ahead = false;
            } else if( 0 == strcmp("-dist_manhatten", argv[i]) ) {
                manhatten_distance_enabled = true;
            } else if( 0 == strcmp("-level", argv[i]) && i+1<argc) {
                start_level = atoi(argv[i+1]);
                ++i;
            } else if( 0 == strcmp("-record", argv[i]) && i+1<argc) {
                record_bmpseq_basename = argv[i+1];
                ++i;
            }
        }
    }
    const std::string exename(argv[0]);
    {
        jau::log_printf("\n%s\n\n", get_usage(exename).c_str());
        jau::log_printf("- 2p %d\n", human_blinky);
        jau::log_printf("- use_audio %d\n", use_audio);
        jau::log_printf("- pixqual %d\n", pixel_filter_quality);
        jau::log_printf("- enable_vsync %d\n", enable_vsync);
        jau::log_printf("- forced_fps %d\n", forced_fps);
        jau::log_printf("- fields_per_sec %5.2f\n", fields_per_sec_total);
        jau::log_printf("- win size %d x %d\n", win_width, win_height);
        jau::log_printf("- show_fps %d\n", log_fps());
        jau::log_printf("- show_modes %d\n", log_modes());
        jau::log_printf("- show_moves %d\n", log_moves());
        jau::log_printf("- show_targets %d\n", show_targets);
        jau::log_printf("- show_debug_gfx %d\n", show_debug_gfx());
        jau::log_printf("- no_ghosts %d\n", disable_all_ghosts);
        jau::log_printf("- invincible %d\n", invincible);
        jau::log_printf("- bugfix %d\n", !use_original_pacman_behavior());
        jau::log_printf("- decision_on_spot %d\n", !use_decision_one_field_ahead());
        jau::log_printf("- distance %s\n", use_manhatten_distance() ? "Manhatten" : "Euclidean");
        jau::log_printf("- level %d\n", get_current_level());
        jau::log_printf("- record %s\n", record_bmpseq_basename.size()==0 ? "disabled" : record_bmpseq_basename.c_str());
    }

    {
        const float origin_norm[] = { 0.5f, 0.5f };
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, std::to_string(pixel_filter_quality).c_str());
        if( !pixel::init_gfx_subsystem(argv[0], "Pacman", win_width, win_height, origin_norm, true, true /* subsys primitives */) ) {
            return 1;
        }
    }

    global_maze = std::make_unique<maze_t>("pacman/playfield_pacman.txt");
    if( !global_maze->is_ok() ) {
        jau::log_printf("Maze: Error: %s\n", global_maze->toString().c_str());
        return -1;
    }
    {
        jau::log_printf("--- 8< ---\n");
        const float maze_width = (float)global_maze->width();
        global_maze->draw( [&maze_width](float x, float y, tile_t tile) {
            fprintf(stderr, "%s", to_string(tile).c_str());
            if( x == maze_width-1 ) {
                fprintf(stderr, "\n");
            }
            (void)y;
        });
        jau::log_printf("--- >8 ---\n");
        jau::log_printf("Maze: %s\n", global_maze->toString().c_str());
    }
    pixel::cart_coord.set_fitting(0.0f, 0.0f, (float)global_maze->px_width(), (float)global_maze->px_height());
    // pixel::cart_coord.set_fitting(0.0f, (float)global_maze->px_height(), (float)global_maze->px_width(), 0.0f);

    jau::log_printf(0, "XX %s\n", pixel::cart_coord.toString().c_str());
    {
        float w = pixel::cart_coord.width();
        float h = pixel::cart_coord.height();
        float r01 = h/w;
        float a = w / h;
        printf("-w %f [x]\n-h %f [y]\n-r1 %f [y/x]\n-r2 %f [x/y]\n", w, h, r01, a);
    }

    current_level = start_level;

    #if !defined(__EMSCRIPTEN__)
        if( use_audio ) {
            jau::audio::init_audio_subsystem(MIX_INIT_OGG, MIX_INIT_OGG);
        }
    #endif
    load_samples();

    pacman_maze_tex = std::make_shared<pixel::texture_t>("pacman/"+global_maze->get_texture_file());
    global_tex = std::make_shared<global_tex_t>();
    pacman_left2_tex = std::make_shared<pixel::texture_t>(*global_tex->all_images(), 0 + 1*13, 28 + 0, 13, 13);

    pacman = std::make_shared<pacman_t>(fields_per_sec_total);
    pacman->set_invincible(invincible);
    jau::log_printf("%s\n", pacman->toString().c_str());

    if( !disable_all_ghosts ) {
        ghosts_.push_back( std::make_shared<ghost_t>(ghost_t::personality_t::BLINKY, fields_per_sec_total) );
        ghosts_.push_back( std::make_shared<ghost_t>(ghost_t::personality_t::PINKY, fields_per_sec_total) );
        ghosts_.push_back( std::make_shared<ghost_t>(ghost_t::personality_t::INKY, fields_per_sec_total) );
        ghosts_.push_back( std::make_shared<ghost_t>(ghost_t::personality_t::CLYDE, fields_per_sec_total) );
        blinky = ghost( ghost_t::personality_t::BLINKY );
        if( human_blinky ) {
            blinky->set_manual_control(true);
        }
    }
    for(const ghost_ref& g : ghosts()) {
        jau::log_printf("%s\n", g->toString().c_str());
    }

    current_level = start_level - 1;
    pacman->reset_score();
    set_game_mode(game_mode_t::NEXT_LEVEL, 1);

    // loop
    #if defined(__EMSCRIPTEN__)
        (void)use_audio;
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
    return 0;
}
