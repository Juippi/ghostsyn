#pragma once

namespace UI {
    namespace Text {
	static constexpr int font_size = 16;
	static constexpr int row_gap = 2;
	static constexpr int char_height = font_size + row_gap;
	static constexpr int char_width = 8;
    };
    namespace Tracker {
	static constexpr int track_pad = 3;
	static constexpr int row_pad = 5;
	
	static constexpr int visible_rows = 15;
	
	static constexpr int default_num_tracks = 8;
	static constexpr int default_num_rows = 48;
    }
};
