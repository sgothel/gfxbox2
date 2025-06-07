/*
 * Highscore.hpp
 *
 *  Created on: Mar 16, 2025
 *      Author: svenson
 */

#ifndef INCLUDE_PIXEL_HIGHSCORE_HPP_
#define INCLUDE_PIXEL_HIGHSCORE_HPP_
#include <pixel/pixel.hpp>
#include <pixel/pixel4f.hpp>
#include <string>
#include <vector>

#include <pixel/pixel2f.hpp>

namespace pixel {
    class HighScore {
      private:
        int resetTextEntries(int lineno, int edit=-1);
        f4::vec_t m_text_color;
      public:
        static constexpr bool debug_on = true;
        
        struct Entry {
            std::string name;
            uint32_t score;
        };
        std::vector<Entry> table;
        static std::vector<pixel::texture_ref> textEntries;

        HighScore(f4::vec_t text_color, const size_t s = 10) noexcept 
        : m_text_color(text_color) {
            reset(s);
        }
        
        HighScore(const HighScore &o) = default;
        
        void reset(size_t s = 10){
            table.clear();
            table.reserve(s);
            while(s-- > 0){
                table.emplace_back("AAA", 0);
            }
            //resetTextEntries(int lineno, f4::vec_t vec4_text_color, int edit=-1);
        }
        
        f2::point_t topLeft() const { 
            return pixel::f2::point_t(pixel::cart_coord.min_x(), pixel::cart_coord.max_y());
        }
        
        void readFile(const std::string& fname);
        
        void write_file(const std::string& fname);
        
        bool addScore(const Entry& p);

        size_t find_idx(const Entry& p) const;
        
        bool goodEnough(const Entry& p) const {
            return p.score >= table[table.size()-1].score;
        }
        
        bool enterEntry(Entry& p, pixel::input_event_t& event);
        void showScores() const;
    };
}


#endif /* INCLUDE_PIXEL_HIGHSCORE_HPP_ */
