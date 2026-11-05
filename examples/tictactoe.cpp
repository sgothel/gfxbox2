/*
 * Author: Hermann Kai Gothel <hkg@jausoft.com>
 * Copyright (c) 2026 Gothel Software e.K.
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
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <pixel/pixel2f.hpp>
#include <pixel/pixel2i.hpp>
#include <pixel/pixel.hpp>
#include <pixel/dynarray2d.hpp>
#include <jau/utils.hpp>

#include <cstdio>
#include <cmath>
#include <algorithm>
#include <vector>

using std::round;

using namespace jau;
static pixel::input_event_t event;

static const float border = 1;
static const float wh = 1.0f;

struct ICoord {
	size_t x, y;
};

constexpr bool operator==(const ICoord& lhs, const ICoord& rhs ) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

class TicTacToe {
  public:
    enum class Value {
		none,
		o,
		x
	};
	static char toChar(Value v) {
		switch(v) {
			case Value::o: return 'o';
			case Value::x: return 'x';
			default: return '?';
		}
	}
  private:
    DynArray2D<Value> m_arena = DynArray2D<Value>(3, 3); ///< [X][Y]
    float m_spaceheight = (float)m_arena.rows() + 2.0f * border;
    float m_spacewidth = (float)m_arena.columns() + 2.0f * border;

    size_t m_cellsToWin = 3;
	size_t m_cursorX=0, m_cursorY=0;
	Value m_turn = Value::o;
	Value m_winner = Value::none;
	std::vector<ICoord> m_winline;
	uint8_t m_cursor_b = 0;
	int8_t m_cursor_coldir=1;

	void drawRaster() {
		pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 0 /* a */);
		{
			for(float ypos=border+wh; ypos<m_spaceheight-border; ypos+=wh) { // NOLINT(bugprone-float-loop-counter)
				pixel::f2::point_t p1(border,    ypos);
				pixel::f2::point_t p2(m_spacewidth-border, ypos);
				pixel::f2::lineseg_t::draw(p1, p2);
			}
			for(float xpos=border+wh; xpos<m_spacewidth-border; xpos+=wh) { // NOLINT(bugprone-float-loop-counter)
				pixel::f2::point_t p1(xpos, border);
				pixel::f2::point_t p2(xpos,  m_spaceheight-border);
				pixel::f2::lineseg_t::draw(p1, p2);
			}
		}
	}

	void drawO(size_t xi, size_t yi) {
		static uint8_t O_b = 0;
		static int8_t O_coldir = 1;
		pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 0 /* a */);
		if (getWinner() == Value::o) {
			bool ofWinner =
				m_winline.cend() != std::find(  // NOLINT
					m_winline.cbegin(), m_winline.cend(), ICoord(xi, yi));
			if(ofWinner) {
				pixel::set_pixel_color(255-O_b /* r */, 255-O_b /* g */, O_b, 0 /* a */);
				if (O_coldir>0 && O_b==255) {
					O_coldir=-1;
				} else if (O_coldir<0 && O_b==0) {
					O_coldir=1;
				}
				const int col = (int)O_b + (int)O_coldir*8;
				O_b = (uint8_t) std::max(0, std::min(255, col));
			}
		}
		{
			float r = wh/2.0f;
			pixel::f2::point_t m { border+r+(float)xi*wh, border+r+(float)yi*wh };
			pixel::f2::vec_t vr { r, r };
			pixel::f2::point_t p0 = m - vr;
		    pixel::f2::point_t p1 = m + vr;
			float step_x = (p1.x - p0.x)/400.0f;
			float step_y = (p1.y - p0.y)/ 400.0f;
		    //pixel::f2::lineseg_t::draw(p0, p1);
			for(float y=p0.y;y<p1.y; y+=step_y) { // NOLINT(bugprone-float-loop-counter)
				for(float x=p0.x;x<p1.x; x+=step_x) { // NOLINT(bugprone-float-loop-counter)
					pixel::f2::point_t p { x, y };
					pixel::f2::vec_t v = p - m;
					const float l = v.length();
					const float d = r-l;
					if(d>=0 && d<0.02f) {
						p.draw();
					}
				}
			}
		}
	}
	void drawX(size_t xi, size_t yi) {
		static uint8_t X_b = 0;
		static int8_t X_coldir = 1;
		pixel::set_pixel_color(0 /* r */, 0 /* g */, 255 /* b */, 0 /* a */);
		if (getWinner() == Value::x) {
			bool ofWinner =
			m_winline.cend() != std::find(  // NOLINT
				m_winline.cbegin(), m_winline.cend(), ICoord(xi, yi));
			if(ofWinner) {
				pixel::set_pixel_color(255-X_b /* r */, 0 /* g */, X_b, 0 /* a */);
				if (X_coldir>0 && X_b==255) {
					X_coldir=-1;
				} else if (X_coldir<0 && X_b==0) {
					X_coldir=1;
				}
				const int col = (int)X_b + (int)X_coldir*8;
				X_b = (uint8_t) std::max(0, std::min(255, col));
			}
		}
		{
			float r = wh/2.0f;
			pixel::f2::point_t m { border+r+(float)xi*wh, border+r+(float)yi*wh };
			pixel::f2::vec_t vr { r, r };
			pixel::f2::point_t p0 = m - vr;
			pixel::f2::point_t p1 = m + vr;
			pixel::f2::vec_t vr2 { r, -r };
			pixel::f2::point_t p2 = m - vr2;
			pixel::f2::point_t p3 = m + vr2;
			const float step = 0.0025f;
			const pixel::f2::vec_t vstepLU {-step, -step};
			for( int i=0;i<10;++i){
				pixel::f2::point_t p2i = p2 + (float)i * vstepLU;
				pixel::f2::point_t p3i = p3 + (float)i * vstepLU;
				pixel::f2::lineseg_t::draw(p2i, p3i);
			}
			const pixel::f2::vec_t vstepRU {step, -step};
			for( int i=0;i<10;++i){
				pixel::f2::point_t p0i = p0 + (float)i * vstepRU;
				pixel::f2::point_t p1i = p1 + (float)i * vstepRU;
				pixel::f2::lineseg_t::draw(p0i, p1i);
			}
		}
	}

  public:
    TicTacToe() noexcept { reset(); }

	void reset() {
		for(size_t y=0; y<m_arena.rows(); ++y) { // NOLINT
			for(size_t x=0; x<m_arena.columns(); ++x) {
				m_arena[x, y] = Value::none;
			}
		}
		m_cursorX=0, m_cursorY=0;
		m_turn = Value::o;
		m_winner = Value::none;
		m_winline.clear();
		m_cursor_b = 0;
		m_cursor_coldir=1;
	}

    float spaceheight() noexcept { return m_spaceheight; }

    void resize(size_t columns, size_t rows) {
        if (columns < 3 || rows < 3) {
            return;
        }
        if (columns != m_arena.columns() || rows != m_arena.rows()) {
            m_arena = DynArray2D<Value>(columns, rows); ///< [X][Y]
            m_spaceheight = (float)m_arena.rows() + 2.0f * border;
            m_spacewidth = (float)m_arena.columns() + 2.0f * border;
            reset();
        }
    }
    size_t cellsToWin() noexcept  { return m_cellsToWin; }
    void setCellsToWin(size_t v) noexcept {
        if (m_cellsToWin < 3) {
            return;
        }
        m_cellsToWin = v;
    }
    void setTicTacToeDefaults() noexcept {
        m_cellsToWin = 3;
        resize(3, 3);
    }
    void setWuZiQiDefaults() noexcept {
        m_cellsToWin = 5;
        resize(10, 10);
    }

	Value getTurn() { return m_turn; }

	size_t columns() { return m_arena.columns(); }
    size_t rows() { return m_arena.rows(); }

  private:
	bool isWinner([[maybe_unused]] Value v, size_t x, size_t y) {
		// direction: horizontal to the right
		if(x + m_cellsToWin <= m_arena.columns() ) {
			m_winline.push_back({x, y});
			size_t xi;
			for(xi = x + 1;
				xi < x + m_cellsToWin && v == m_arena[xi, y];
				++xi)
			{ m_winline.push_back({xi, y}); }
			if( xi - x == m_cellsToWin ) {
				return true;
			} else {
				m_winline.clear();
			}
		}
		// direction: vertical to the top
		if(y + m_cellsToWin <= m_arena.rows()) {
			m_winline.push_back({x, y});
			size_t yi;
			for(yi = y + 1;
				yi < y + m_cellsToWin && v == m_arena[x, yi];
				++yi)
			{ m_winline.push_back({x, yi}); }
			if( yi - y == m_cellsToWin ) {
				return true;
			} else {
				m_winline.clear();
			}
		}
		// direction: diagonal to the top-right
		if(x + m_cellsToWin <= m_arena.columns() && y + m_cellsToWin <= m_arena.rows()) {
			m_winline.push_back({x, y});
			size_t yi, xi;
			for(xi = x + 1, yi = y + 1;
				xi < x + m_cellsToWin && yi < y + m_cellsToWin &&
				v == m_arena[xi, yi];
				++xi, ++yi)
			{ m_winline.push_back({xi, yi}); }
			if( xi == x + m_cellsToWin && yi == y + m_cellsToWin) {
				return true;
			} else {
				m_winline.clear();
			}
            /**
	        if( v == m_arena[y + 1][x + 1] ) {
	            if( v == m_arena[y + 2][x + 2] ) {
	                return true;
	            }
	        } */
		}
		// direction: diagonal to the top-left
		if(x + 1 >= m_cellsToWin && y + m_cellsToWin <= m_arena.rows()) {
			m_winline.push_back({x, y});
			size_t xi, yi;
			for(xi = x, yi = y + 1;
				(--xi) + m_cellsToWin > x && yi < y + m_cellsToWin && // NOLINT
				v == m_arena[xi, yi];
				++yi)
			{ m_winline.push_back({xi, yi}); }
			if( x == xi + m_cellsToWin && yi == y + m_cellsToWin ) {
				return true;
			} else {
				m_winline.clear();
			}
			/**
	        if( v == m_arena[y + 1][x - 1] ) {
	            if( v == m_arena[y + 2][x - 2] ) {
	                return true;
	            }
	        } */
		}
		return false;
	}
	void updateWinner() noexcept {
		for(size_t x=0; x<m_arena.columns(); ++x) { // NOLINT
			for(size_t y=0; y<m_arena.rows(); ++y) {
				[[maybe_unused]] Value v = m_arena[x, y];
				if (v != Value::none) {
					if (isWinner(v, x, y)) {
						m_winner = v;
						return;
					}
				}
			}
		}
	}

  public:
	Value getWinner() noexcept { return m_winner; }

	void moveCursorUp(Value player) {
		if (getWinner() != Value::none) { return; }
		if(m_turn==player && m_cursorY+1 < m_arena.rows()) {//FIXME
			++m_cursorY;
		}
	}
	void moveCursorDown(Value player) {
		if (getWinner() != Value::none) { return; }
		if(m_turn==player && m_cursorY>0) {
			--m_cursorY;
		}
	}
	void moveCursorRight(Value player) {
		if (getWinner() != Value::none) { return; }
		if(m_turn==player &&m_cursorX+1 < m_arena.columns()) {//FIXME
			++m_cursorX;
		}
	}
	void moveCursorLeft(Value player) {
		if (getWinner() != Value::none) { return; }
		if(m_turn==player &&m_cursorX>0) {
			--m_cursorX;
		}
	}
	void enter(Value player) {
		if (getWinner() != Value::none) { return; }
		if(m_turn!=player) {
			return;
		}
		const size_t x = m_cursorX, y = m_cursorY;
		if( m_arena[x, y] == Value::none ) {
			m_arena[x, y] = m_turn;
			updateWinner();
			if (m_turn == Value::o) {
				m_turn = Value::x;
			} else {
				m_turn = Value::o;
			}
		}
	}
	void draw() {
		drawRaster();
		for(size_t x=0; x<m_arena.columns(); ++x) {
			for(size_t y=0; y<m_arena.rows(); ++y) {
				if( m_arena[x, y] == Value::o ) {
					drawO(x, y);
				} else if( m_arena[x, y] == Value::x ) {
					drawX(x, y);
				}
			}
		}
		if (getWinner() == Value::none) {
			pixel::set_pixel_color(255-m_cursor_b /* r */, 255-m_cursor_b /* g */, m_cursor_b, 0 /* a */);
			if (m_cursor_coldir>0 && m_cursor_b==255) {
				m_cursor_coldir=-1;
				// printf("XXX r->l %d\n", m_cursor_b);
			} else if (m_cursor_coldir<0 && m_cursor_b==0) {
				m_cursor_coldir=1;
				// printf("XXX l->r %d\n", m_cursor_b);
			}
			const int col = (int)m_cursor_b + (int)m_cursor_coldir*8;
			m_cursor_b = (uint8_t) std::max(0, std::min(255, col));
			{
				pixel::f2::rect_t c{{border+(float)m_cursorX*wh,
					                 border+wh+(float)m_cursorY*wh}, wh, wh};
		 		c.draw();
			}
		}
	}
};
static TicTacToe game;

void mainloop() {
    static uint64_t t_last = getElapsedMillisecond(); // [ms]
    static bool animating = true;

    while(pixel::handle_one_event(event)){
        if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_CLOSE_REQ ) ) {
            printf("Exit Application\n");
            #if defined(__EMSCRIPTEN__)
                emscripten_cancel_main_loop();
            #else
                exit(0);
            #endif
        } else if( event.pressed_and_clr( pixel::input_event_type_t::WINDOW_RESIZED ) ) {
            // nop for this demo, resize already performed
			//pixel::cart_coord.set_height(-spaceheight/2, spaceheight/2);
			pixel::cart_coord.set_height2(0, game.spaceheight());
		} else if( event.released_and_clr(pixel::input_event_type_t::RESET) ) {
			game.reset();
        } else if( event.has_any_p1() ) {
			if( event.released_and_clr( pixel::input_event_type_t::P1_UP ) ) {
				game.moveCursorUp(TicTacToe::Value::o);
			} else if( event.released_and_clr( pixel::input_event_type_t::P1_DOWN ) ) {
				game.moveCursorDown(TicTacToe::Value::o);
			} else if( event.released_and_clr( pixel::input_event_type_t::P1_LEFT ) ) {
				game.moveCursorLeft(TicTacToe::Value::o);
			} else if( event.released_and_clr( pixel::input_event_type_t::P1_RIGHT ) ) {
				game.moveCursorRight(TicTacToe::Value::o);
			} else if( event.released_and_clr(pixel::input_event_type_t::P1_ACTION1) ) {
				game.enter(TicTacToe::Value::o);
			}
		} else if( event.has_any_p2() ) {
			if( event.released_and_clr( pixel::input_event_type_t::P2_UP ) ) {
				game.moveCursorUp(TicTacToe::Value::x);
			} else if( event.released_and_clr( pixel::input_event_type_t::P2_DOWN ) ) {
				game.moveCursorDown(TicTacToe::Value::x);
			} else if( event.released_and_clr( pixel::input_event_type_t::P2_LEFT ) ) {
				game.moveCursorLeft(TicTacToe::Value::x);
			} else if( event.released_and_clr( pixel::input_event_type_t::P2_RIGHT ) ) {
				game.moveCursorRight(TicTacToe::Value::x);
			} else if( event.released_and_clr(pixel::input_event_type_t::P2_ACTION1) ) {
				game.enter(TicTacToe::Value::x);
			}
		}
        if( event.released_and_clr( pixel::input_event_type_t::F1) ) {
            game.setTicTacToeDefaults();
            pixel::cart_coord.set_height2(0, game.spaceheight());
        } else if( event.released_and_clr( pixel::input_event_type_t::F2) ) {
            game.setWuZiQiDefaults();
            pixel::cart_coord.set_height2(0, game.spaceheight());
        } else if( event.released_and_clr( pixel::input_event_type_t::F5 ) ) {
            game.resize(game.columns()-1, game.rows()-1);
            pixel::cart_coord.set_height2(0, game.spaceheight());
        } else if( event.released_and_clr( pixel::input_event_type_t::F6) ) {
            game.resize(game.columns()+1, game.rows()+1);
            pixel::cart_coord.set_height2(0, game.spaceheight());
        } else if( event.released_and_clr( pixel::input_event_type_t::F7) ) {
            game.setCellsToWin(game.cellsToWin()-1);
        } else if( event.released_and_clr( pixel::input_event_type_t::F8) ) {
            game.setCellsToWin(game.cellsToWin()+1);
        }
        if( event.paused() ) {
            animating = false;
        } else {
            if( !animating ) {
                t_last = getElapsedMillisecond(); // [ms]
            }
            animating = true;
        }
    }
    const uint64_t t1 = getElapsedMillisecond(); // [ms]
    const float dt = (float)( t1 - t_last ) / 1000.0f; // [s]
    t_last = t1;

    pixel::set_pixel_color(0 /* r */, 255 /* g */, 0 /* b */, 255 /* a */);
    pixel::texture_ref hud_text = pixel::make_text("td %s, fps %2.2f, dt %d ms, %s",
            to_decstring(t1, ',', 9).c_str(), pixel::gpu_avg_fps(), (int)(dt*1000), animating?"animating":"paused");

    // white background
    pixel::clear_pixel_fb(255, 255, 255, 255);

	game.draw();

	pixel::set_pixel_color(0 /* r */, 0 /* g */, 255 /* b */, 0 /* a */);
	if(!event.paused()){
	}
    pixel::swap_pixel_fb(false);
    if( nullptr != hud_text ) {
        hud_text->draw_fbcoord(0, 0);
    }
	pixel::set_pixel_color(0 /* r */, 0 /* g */, 0 /* b */, 255 /* a */);
	pixel::texture_ref hud2 = pixel::make_text("Turn %c, Winner %c (need %zu)",
	        TicTacToe::toChar(game.getTurn()),
			TicTacToe::toChar(game.getWinner()), game.cellsToWin());
	if( nullptr != hud2 ) {
		hud2->draw_fbcoord(0, pixel::fb_height - hud2->height);
		//hud2->draw(0, 1);
	}

    pixel::swap_gpu_buffer();
}

int main(int argc, char *argv[])
{
    int window_width = 1920, window_height = 1000;
    #if defined(__EMSCRIPTEN__)
        window_width = 1024, window_height = 576; // 16:9
    #endif
    size_t g_columns = game.columns(), g_rows = game.rows();
    {
        for(int i=1; i<argc; ++i) {
            if (0 == strcmp("-width", argv[i]) && i + 1 < argc) {
                window_width = atoi(argv[i + 1]);
                ++i;
            } else if (0 == strcmp("-height", argv[i]) && i + 1 < argc) {
                window_height = atoi(argv[i + 1]);
                ++i;
            } else if (0 == strcmp("-columns", argv[i]) && i + 1 < argc) {
                g_columns = atoi(argv[i + 1]);
                                ++i;
            } else if (0 == strcmp("-rows", argv[i]) && i + 1 < argc) {
                g_rows = atoi(argv[i + 1]);
                ++i;
            } else if (0 == strcmp("-wuziqi", argv[i])) {
                game.setWuZiQiDefaults();
            }
        }
    }
    game.resize(g_columns, g_rows);
    {
        const float origin_norm[] = { 0.0f, 1.0f };
        if( !pixel::init_gfx_subsystem(argv[0], "Tic-tac-toe or 五子棋 (wu zi qi)", window_width, window_height, origin_norm, true, true /* subsys primitives */) ) {
            return 1;
        }
		// pixel::cart_coord.set_height(-spaceheight/2, spaceheight/2);
		pixel::cart_coord.set_height2(0, game.spaceheight());
    }

    log_printf(0, "XX %s\n", pixel::cart_coord.toString().c_str());
    {
        float w = pixel::cart_coord.width();
        float h = pixel::cart_coord.height();
        float r01 = h/w;
        float a = w / h;
        printf("-w %f [x]\n-h %f [y]\n-r1 %f [y/x]\n-r2 %f [x/y]\n", w, h, r01, a);
    }

    #if defined(__EMSCRIPTEN__)
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while( true ) { mainloop(); }
    #endif
}
