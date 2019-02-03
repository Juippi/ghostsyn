#include "tracker_window.hpp"
#include "utils.hpp"
#include <iostream>
#include <string>
#include <functional>
#include <boost/range/irange.hpp>

using boost::irange;
using namespace UI;

TrackerWindow::TrackerWindow(int x, int y, int width, int height, SDL_Renderer *renderer,
			     TTF_Font *text_font, ModalPromptInterface &prompt_input_,
			     TrackerData &data_)
    : EditorWindow(x, y, width, height, renderer, text_font, prompt_input_),
      data(data_),
      cursor_area(TrackerWindow::CURSOR_AREA_TRACKER),
      order_shrink("-",
		   std::bind(&TrackerWindow::order_adjust, this, -1),
		   850, (Text::font_size * 2)),
      order_grow("+",
		 std::bind(&TrackerWindow::order_adjust, this, 1),
		 850 + 70, (Text::font_size * 2)),
      tempo_textbox(Text::char_width * 4 * 9,
		    Text::font_size * 2 * 6,
		    6,
		    tostr(data.ticklen),
		    std::bind(&TrackerWindow::set_tempo_cb, this,
			      std::placeholders::_1)),
      pattern_rows_textbox(Text::char_width * 4 * 9,
			   Text::font_size * 2 * 7,
			   6,
			   tostr(data.num_rows),
			   std::bind(&TrackerWindow::set_pattern_rows_cb, this,
				     std::placeholders::_1)),
      pattern_tracks_textbox(Text::char_width * 4 * 9,
			   Text::font_size * 2 * 8,
			   6,
			   tostr(data.num_tracks),
			   std::bind(&TrackerWindow::set_pattern_tracks_cb, this,
				     std::placeholders::_1)),
      master_hb_coeff_textbox(Text::char_width * 4 * 9,
			      Text::font_size * 2 * 9,
			      6,
			      tostr(data.master_hb_coeff),
			      std::bind(&TrackerWindow::set_master_hb_coeff_cb, this,
					std::placeholders::_1)),
      master_hb_mix_textbox(Text::char_width * 4 * 9,
			    Text::font_size * 2 * 10,
			    6,
			    tostr(data.master_hb_coeff),
			    std::bind(&TrackerWindow::set_master_hb_mix_cb, this,
				      std::placeholders::_1))
{
    register_(order_shrink);
    register_(order_grow);
    register_(tempo_textbox);
    register_(pattern_rows_textbox);
    register_(pattern_tracks_textbox);
    register_(master_hb_coeff_textbox);
    register_(master_hb_mix_textbox);
}

void TrackerWindow::key_down(const SDL_Keysym &sym) {
    EditorWindow::key_down(sym);

    // no numpad
    if (cursor_area == CURSOR_AREA_TRACKER) {
	if (sym.mod & KMOD_CTRL && sym.mod & KMOD_SHIFT) {
	    switch(sym.sym) {
	    case SDLK_c:
		copied_pattern = data.get_pattern(current_pattern);
		changed = true;
		break;
	    case SDLK_x:
		copied_pattern = data.get_pattern(current_pattern);
		data.clear_pattern(current_pattern);
		changed = true;
		break;
	    case SDLK_v:
		if (copied_pattern.has_value()) {
		    data.set_pattern(current_pattern, copied_pattern.value());
		    changed = true;
		}
		break;
	    case SDLK_m:
	    {
		bool all_unmuted = true;
		for (auto i : irange(0u, data.num_tracks)) {
		    if (mute_flags[i]) {
			all_unmuted = false;
			break;
		    }
		}
		bool new_mute = all_unmuted;
		for (auto i : irange(0u, data.num_tracks)) {
		    mute_flags[i] = new_mute;
		}
		changed = true;
	    }
	    break;
	    }

	} else if (sym.mod & KMOD_CTRL) {
	    switch (sym.sym) {
	    case SDLK_0:
		current_instrument = 0;
		changed = true;
		break;
	    case SDLK_1:
		current_instrument = 1;
		changed = true;
		break;
	    case SDLK_n:
		if (data.patterns.size() > 0 &&
		    current_pattern < data.patterns.size() - 1) {
		    ++current_pattern;
		    changed = true;
		}
		break;
	    case SDLK_p:
		if (current_pattern > 0) {
		    --current_pattern;
		    changed = true;
		}
		break;
	    case SDLK_u:
		change_octave(1);
		changed = true;
		break;
	    case SDLK_d:
		change_octave(-1);
		changed = true;
		break;
	    case SDLK_KP_PLUS:
	    case SDLK_PLUS:
		data.new_pattern();
		changed = true;
		break;
	    case SDLK_KP_MINUS:
	    case SDLK_MINUS:
		if (data.del_pattern(current_pattern)) {
		    if (current_pattern >= data.patterns.size()) {
			current_pattern = data.patterns.size() - 1;
		    }
		    changed = true;
		}
		break;
	    case SDLK_o:
		cursor_area = CURSOR_AREA_ORDER;
		changed = true;
		break;
	    case SDLK_m:
		mute_flags[cursor_x / 4] = !mute_flags[cursor_x / 4];
		changed = true;
		break;
	    case SDLK_l:
	    {
		// Toggle solo: if current track is already soloed (only non-muted),
		// unmute all tracks, otherwise unmute all other tracks
		bool solo = false;
		for (auto i : irange(0u, data.num_tracks)) {
		    if (!mute_flags[i] &&
			static_cast<int>(i) != (cursor_x / 4)) {
			solo = true;
			break;
		    }
		}
		for (auto i : irange(0u, data.num_tracks)) {
		    if (solo) {
			if (static_cast<int>(i) == (cursor_x / 4)) {
			    mute_flags[i] = false;
			} else {
			    mute_flags[i] = true;
			}
		    } else {
			mute_flags[i] = false;
		    }
		}
		changed = true;
	    }
	    break;

	    // Helpers to kbds without numpad
	    case SDLK_LEFT:
		change_pattern(-1);
		changed = true;
		break;
	    case SDLK_RIGHT:
		change_pattern(1);
		changed = true;
		break;
	    case SDLK_DOWN:
		change_octave(-1);
		changed = true;
		break;
	    case SDLK_UP:
		change_octave(1);
		changed = true;
		break;

	    // CTRL-[xcv] : cut/copy/paste track
	    case SDLK_c:
		copied_track = data.get_track(current_pattern, cursor_x / 4);
		changed = true;
		break;
	    case SDLK_x:
		copied_track = data.get_track(current_pattern, cursor_x / 4);
		data.clear_track(current_pattern, cursor_x / 4);
		changed = true;
		break;
	    case SDLK_v:
		if (copied_track.has_value()) {
		    data.set_track(current_pattern, cursor_x / 4, copied_track.value());
		    changed = true;
		}
		break;
	    }

	} else { // no CTRL, no SHIFT

	    switch (sym.sym) {
	    case SDLK_UP:
		cursor_move(0, -1);
		changed = true;
		break;
	    case SDLK_DOWN:
		cursor_move(0, 1);
		changed = true;
		break;
	    case SDLK_LEFT:
		cursor_move(-1, 0);
		changed = true;
		break;
	    case SDLK_RIGHT:
		cursor_move(1, 0);
		changed = true;
		break;
	    case SDLK_PAGEUP:
		cursor_move(0,  -8);
		changed = true;
		break;
	    case SDLK_PAGEDOWN:
		cursor_move(0,  8);
		changed = true;
		break;
	    case SDLK_HOME:
		cursor_move(0, -1000);
		changed = true;
		break;
	    case SDLK_END:
		cursor_move(0, 1000);
		changed = true;
		break;
	    case SDLK_TAB:
		if (sym.mod & KMOD_SHIFT) {
		    cursor_move(-4, 0);
		} else {
		    cursor_move(4, 0);
		}
		changed = true;
		break;
	    case SDLK_KP_DIVIDE:
		change_octave(-1);
		changed = true;
		break;
	    case SDLK_KP_MULTIPLY:
		change_octave(+1);
		changed = true;
		break;
	    case SDLK_DELETE:
		if (edit(-1)) {
		    cursor_move(0, 1);
		    changed = true;
		}
		break;
	    case SDLK_KP_PLUS:
		change_pattern(1);
		changed = true;
		break;
	    case SDLK_KP_MINUS:
		change_pattern(-1);
		changed = true;
		break;
	    case SDLK_KP_0:
		current_instrument = 0;
		changed = true;
		break;
	    case SDLK_KP_1:
		current_instrument = 1;
		changed = true;
		break;
	    default:
		if (edit(sym.sym)) {
		    cursor_move(0, 1);
		    changed = true;
		}
		break;
	    }
	}

    } else if (cursor_area == CURSOR_AREA_ORDER) {

	if (sym.mod & KMOD_CTRL) {
	    switch (sym.sym) {
	    case SDLK_o:
		cursor_area = CURSOR_AREA_TRACKER;
		changed = true;
		break;
	    }
	} else {
	    switch (sym.sym) {
	    case SDLK_LEFT:
		order_cursor_move(-1);
		changed = true;
		break;
	    case SDLK_RIGHT:
		order_cursor_move(1);
		cursor_move(1, 0);
		changed = true;
		break;
	    case SDLK_MINUS:
	    case SDLK_KP_MINUS:
		current_order_adjust(-1);
		changed = true;
		break;
	    case SDLK_PLUS:
	    case SDLK_KP_PLUS:
		current_order_adjust(1);
		changed = true;
		break;
	    case SDLK_PAGEDOWN:
		current_order_adjust(-10);
		changed = true;
		break;
	    case SDLK_PAGEUP:
		current_order_adjust(10);
		changed = true;
		break;
	    case SDLK_DELETE:
		order_del();
		changed = true;
		break;
	    case SDLK_INSERT:
		order_insert();
		changed = true;
		break;
	    case SDLK_1:
	    case SDLK_2:
	    case SDLK_3:
	    case SDLK_4:
	    case SDLK_5:
	    case SDLK_6:
	    case SDLK_7:
	    case SDLK_8:
	    case SDLK_9:
		current_order_set(sym.sym - SDLK_1);
		changed = true;
		break;
	    }
	}
    }
}

void TrackerWindow::key_up(const SDL_Keysym &sym) {
    EditorWindow::key_up(sym);
}

void TrackerWindow::update() {
    const int TRACKER_START_Y = (768 -
				 ((Text::font_size * 2) + Tracker::row_pad) *
				 (Tracker::visible_rows - Tracker::row_pad));

    const std::vector<std::string> notes = {"C-", "C#", "D-", "D#",
					    "E-", "F-", "F#", "G-",
					    "G#", "A-", "A#", "H-"};

    draw_rect(1, TRACKER_START_Y, width - 2, height - TRACKER_START_Y - 1,
	      Color(80, 80, 40, 127));

    data.lock();

    int track_width;
    bool hw_effects;
    bool hw_notes;
    if (data.num_tracks <= 4) {
	track_width = Text::char_width * 2 * 14;
	hw_effects = false;
	hw_notes = false;
    } else if (data.num_tracks <= 6) {
	track_width = Text::char_width * 2 * 10;
	hw_effects = true;
	hw_notes = false;
    } else {
	track_width = Text::char_width * 2 * 7;
	hw_effects = true;
	hw_notes = true;
    }

    auto &patt = data.patterns[current_pattern];
    for (int disp_row = 0; disp_row < Tracker::visible_rows; ++disp_row) {
	int row_idx = cursor_row - (Tracker::visible_rows / 2) + disp_row;
	if (row_idx < 0 || static_cast<size_t>(row_idx) >= data.num_rows) {
	    continue;
	}

	std::stringstream lineno;
	lineno.width(2);
	lineno.fill('0');
	lineno << std::hex << row_idx;

	draw_text_hw(4, TRACKER_START_Y + Tracker::row_pad + ((Text::font_size * 2) + Tracker::row_pad) * disp_row,
		     lineno.str(), Color(160, 160, 160, 255));

	for (auto track_idx : irange(0, static_cast<int>(data.num_tracks))) {
            auto &track = patt.tracks[track_idx];
	    Pattern::Cell &cell = track.cells[row_idx];

	    int start_x = (40 + Tracker::track_pad +
			   (track_width + Tracker::track_pad) * track_idx);
	    int start_y = TRACKER_START_Y + Tracker::row_pad + ((Text::font_size * 2) + Tracker::row_pad) * disp_row;

	    Color cell_color;
	    if (mute_flags[track_idx]) {
		cell_color = Colors::tracker_bg;
	    } else if (row_idx == cursor_row) {
		cell_color = Colors::tracker_cursor_row;
	    } else if (playing_pattern == current_pattern &&
		       playing_row == static_cast<unsigned int>(row_idx)) {
		cell_color = Colors::tracker_play_cursor;
	    } else if (row_idx % 2 == 0) {
		cell_color = Colors::tracker_row_hilight1;
	    } else {
		cell_color = Colors::tracker_bg;
	    }

	    std::stringstream r_text;
	    if (cell.note == -1) {
		r_text << "...";
	    } else if (cell.note == 1 && cell.octave == 0) {
		r_text << "===";
	    } else {
		r_text << notes[cell.note] << cell.octave;
	    }
	    if (hw_notes) {
	      draw_text_hw(start_x, start_y, r_text.str(),
			   Colors::default_color_fg, cell_color);
	    } else {
		draw_text(start_x, start_y, r_text.str(),
			  Colors::default_color_fg, cell_color);
	    }

	    r_text.str(std::string());
	    if (!hw_effects) {
		r_text << " ";
	    }
	    r_text << cell.fx;
	    r_text.width(2);
	    r_text.fill('0');
	    r_text << std::hex << cell.param;
	    {
		int x;
		if (hw_notes) {
		    x = start_x + Text::char_width * 2 * 4;
		} else {
		    x = start_x + Text::char_width * 2 * 7;
		}
		if (hw_effects) {
		    draw_text_hw(x, start_y, r_text.str(),
				 Colors::default_color_fg, cell_color);
		} else {
		    draw_text(x, start_y, r_text.str(),
			      Colors::default_color_fg, cell_color);
		}
	    }

	    // instrument number
	    if (cell.note > 0 || cell.octave > 0) {
		std::stringstream instr_num_text;
		instr_num_text << cell.instrument;
		int x;
		if (hw_notes) {
		    x = start_x + Text::char_width * 2 * 3;
		} else {
		    x = start_x + Text::char_width * 4 * 3;
		}
		draw_text_hw(x, start_y, instr_num_text.str(),
			     Colors::tracker_instrument_num, cell_color);
	    }

	    // cursor
	    if (cursor_area == CURSOR_AREA_TRACKER &&
		row_idx == cursor_row &&
		cursor_x / 4 == track_idx) {
		Rect cursor_rect;
		if (cursor_x % 4 == 0) {
		    cursor_rect.x = start_x;
		    if (hw_notes) {
			cursor_rect.w = (Text::char_width * 2) * 3;
		    } else {
			cursor_rect.w = (Text::char_width * 4) * 3;
		    }
		} else {
		    if (hw_effects) {
			if (hw_notes) {
			    cursor_rect.x = start_x + (Text::char_width * 2) * (3 + cursor_x % 4);
			} else {
			    cursor_rect.x = start_x + (Text::char_width * 2) * (6 + cursor_x % 4);
			}
			cursor_rect.w = (Text::char_width * 2);
		    } else {
			// TODO: ignore hw_notes here for now, at the moment can't happen
			cursor_rect.x = start_x + (Text::char_width * 4) * (3 + cursor_x % 4);
			cursor_rect.w = (Text::char_width * 4);
		    }
		}
		cursor_rect.y = start_y;
		cursor_rect.h = (Text::font_size * 2);
		draw_rect(cursor_rect);
	    }

	    ++track_idx;
	}
    }

    // info area
    std::stringstream p_text;
    p_text << "Pattern: " << (current_pattern + 1)
	   << "/" << data.patterns.size();
    draw_text(0, 0, p_text.str());

    std::stringstream o_text;
    o_text << "Order  :";
    draw_text(0, (Text::font_size * 2), o_text.str());
    int order_idx = 0;
    for (auto n : data.order) {
	o_text.clear();
	o_text.str(std::string());
	if (n < 9) {
	    o_text << " ";
	}
	o_text << (n + 1);
	if ((order_idx + order_idx / 18) % 2 == 0) {
	    draw_text_hw(order_x(order_idx), order_y(order_idx), o_text.str(),
			 Colors::default_color_fg, Colors::tracker_order_bg_1);
	} else {
	    draw_text_hw(order_x(order_idx), order_y(order_idx), o_text.str(),
			 Colors::default_color_fg, Colors::tracker_order_bg_2);
	}
	++order_idx;
    }

    // Order cursor
    if (cursor_area == CURSOR_AREA_ORDER) {
	draw_rect(order_x(order_cursor_pos), order_y(order_cursor_pos),
		  Text::char_width * 4, Text::font_size * 2);
    }

    std::stringstream oct_text;
    oct_text << "Octave   " << current_octave;
    draw_text(0, (Text::font_size * 2) * 4, oct_text.str());

    std::stringstream instr_text;
    instr_text << "Instr    " << current_instrument;
    draw_text(0, (Text::font_size * 2) * 5, instr_text.str());

    std::stringstream tempo_text;
    tempo_text << "Tempo   ";
    draw_text(0, (Text::font_size * 2) * 6, tempo_text.str());

    std::stringstream rows_text;
    rows_text << "Rows    ";
    draw_text(0, (Text::font_size * 2) * 7, rows_text.str());

    std::stringstream tracks_text;
    tracks_text << "Tracks  ";
    draw_text(0, (Text::font_size * 2) * 8, tracks_text.str());

    std::stringstream hb_width_text;
    hb_width_text << "HB width";
    draw_text(0, (Text::font_size * 2) * 9, hb_width_text.str());

    std::stringstream hb_gain_text;
    hb_gain_text << "HB gain";
    draw_text(0, (Text::font_size * 2) * 10, hb_gain_text.str());

    // Song length
    long long samples = data.order.size() * data.num_rows * data.ticklen;
    long long seconds = samples / (44100 * 2);
    char buf[10];
    snprintf(buf, 10, "%02lld:%02lld", seconds / 60, seconds % 60);
    draw_text(width - Text::char_width * 5 * 4, 0, buf);

    data.unlock();
    EditorWindow::update();
}

void TrackerWindow::cursor_move(int x, int y) {
    if (x < 0) {
	cursor_x = std::max(0, cursor_x + x);
    } else {
	cursor_x = std::min(static_cast<int>(data.num_tracks * 4) - 1, cursor_x + x);
    }
    if (y < 0) {
	cursor_row = std::max(0, cursor_row + y);
    } else {
	cursor_row = std::min(static_cast<int>(data.num_rows) - 1, cursor_row + y);
    }
}

void TrackerWindow::cursor_move_track(int dir) {
    if (dir > 0) {
	if (cursor_x < static_cast<int>(data.num_tracks) * 4) { // note col of last track
	    cursor_x = (cursor_x / 4 + 1) * 4;
	    if (cursor_x > static_cast<int>(data.num_tracks) * 4 - 1) {
		cursor_x = data.num_tracks * 4 - 1;
	    }
	}
    } else if (dir < 0) {
	cursor_x = ((cursor_x - 1 ) / 4) * 4;
	if (cursor_x < 0) {
	    cursor_x = 0;
	}
    }
}

bool TrackerWindow::edit(int key_char) {
    std::cerr << "EDIT " << key_char << std::endl;

    struct _note {
	int key;
	int oct;
	int note;
    };
    struct _note notes[] = {
	{'z', 0, 0},
	{'s', 0, 1},
	{'x', 0, 2},
	{'d', 0, 3},
	{'c', 0, 4},
	{'v', 0, 5},
	{'g', 0, 6},
	{'b', 0, 7},
	{'h', 0, 8},
	{'n', 0, 9},
	{'j', 0, 10},
	{'m', 0, 11},

	{',', 1, 0},
	{'l', 1, 1},
	{'.', 1, 2},

	{'q', 1, 0},
	{'2', 1, 1},
	{'w', 1, 2},
	{'3', 1, 3},
	{'e', 1, 4},
	{'r', 1, 5},
	{'5', 1, 6},
	{'t', 1, 7},
	{'6', 1, 8},
	{'y', 1, 9},
	{'7', 1, 10},
	{'u', 1, 11},

	{'i', 2, 0},
	{'9', 2, 1},
	{'o', 2, 2},
	{'0', 2, 3},
	{'p', 2, 4},

	{-1, -1, -1}
    };

    struct _digit {
	int key;
	int val;
    };
    struct _digit hexdigits[] {
	{'0', 0},
	{'1', 1},
	{'2', 2},
	{'3', 3},
	{'4', 4},
	{'5', 5},
	{'6', 6},
	{'7', 7},
	{'8', 8},
	{'9', 9},
	{'a', 10},
	{'b', 11},
	{'c', 12},
	{'d', 13},
	{'e', 14},
	{'f', 15},
	{-1, -1}
    };

    data.lock();
    int track = cursor_x / 4;
    Pattern::Cell &c = data.patterns[current_pattern].tracks[track].cells[cursor_row];
    if (cursor_x % 4 == 0) {
	// edit note
	if (key_char == -1) {
	    c.note = -1;
	    c.octave = 0;
	    data.unlock();
	    return true;
	} else if (key_char == ' ') { // note off
	    c.note = 1;
	    c.octave = 0;
	    c.instrument = 0;
	    data.unlock();
	    return true;
	}
	for (auto *note = notes; note->key != -1; ++note) {
	    if (key_char == note->key) {
		unsigned int note_octave = note->oct + current_octave;
		if (note_octave > octave_max) {
		    // max octave limit
		    data.unlock();
		    return false;
		}
		c.note = note->note;
		c.octave = note_octave;
		c.instrument = current_instrument;
		data.unlock();
		return true;
	    }
	}
    } else {
	// edit effect value
	for (auto *digit = hexdigits; digit->key != -1; ++digit) {
	    if (key_char == digit->key || key_char == -1) {
		int new_val = key_char == -1 ? 0 : digit->val;
		if (cursor_x % 4 == 1) {
		    c.fx = new_val;
		} else if (cursor_x % 4 == 2) {
		    c.param = c.param % 16 + new_val * 16;
		} else {
		    c.param = c.param / 16 * 16 + new_val;
		}
		data.unlock();
		return true;
	    }
	}
    }
    data.unlock();
    return false;
}

void TrackerWindow::change_octave(int dir) {
    if (dir > 0 && current_octave < octave_max) {
	++current_octave;
    } else if (dir < 0 && current_octave > octave_min) {
	--current_octave;
    }
}

void TrackerWindow::change_pattern(int dir) {
    if (dir > 0 && current_pattern + 1 < data.patterns.size()) {
	++current_pattern;
    } else if (dir < 0 && current_pattern > 0) {
	--current_pattern;
    }
}

void TrackerWindow::order_cursor_move(int dir) {
    data.lock();
    if (dir > 0) {
	order_cursor_pos = std::min(order_cursor_pos + 1,
				    static_cast<int>(data.order.size()) - 1);
    } else {
	order_cursor_pos = std::max(0, order_cursor_pos - 1);
    }
    data.unlock();
}

void TrackerWindow::current_order_adjust(int dir) {
    data.lock();
    int order_size = static_cast<int>(data.order.size());
    if (order_cursor_pos >= 0 && order_cursor_pos < order_size) {
	if (dir > 0) {
	    data.order[order_cursor_pos] = std::min(static_cast<int>(data.patterns.size()) - 1,
						    data.order[order_cursor_pos] + dir);
	} else {
	    data.order[order_cursor_pos] = std::max(0, data.order[order_cursor_pos] + dir);
	}
    }
    data.unlock();
}

void TrackerWindow::current_order_set(int pattern) {
    data.lock();
    int order_size = static_cast<int>(data.order.size());
    int patterns_size = static_cast<int>(data.patterns.size());
    if (order_cursor_pos >= 0 && order_cursor_pos < order_size &&
	pattern >= 0 && pattern < patterns_size) {
	data.order[order_cursor_pos] = pattern;
    }
    data.unlock();
}

void TrackerWindow::order_adjust(int dir) {
    data.lock();
    if (dir > 0) {
	data.order.push_back(0);
    } else if (dir < 0 && data.order.size() > 1) {
	data.order.resize(data.order.size() - 1);
	if (order_cursor_pos > static_cast<int>(data.order.size()) - 1) {
	    order_cursor_pos = static_cast<int>(data.order.size()) - 1;
	}
    }
    data.unlock();
    changed = true;
}

void TrackerWindow::order_insert() {
    data.lock();
    auto pos = data.order.begin() + order_cursor_pos;
    data.order.insert(pos, current_pattern);
    data.unlock();
}

void TrackerWindow::order_del() {
    data.lock();
    if (data.order.size() > 1) {
	auto pos = data.order.begin() + order_cursor_pos;
	data.order.erase(pos);
	if (order_cursor_pos >= static_cast<int>(data.order.size())) {
	    order_cursor_pos = data.order.size() - 1;
	}
    }
    data.unlock();
}

int TrackerWindow::order_x(int order_idx) {
    return (Text::char_width * 4) / 2 * (16 + (order_idx % 18) * 2);
}

int TrackerWindow::order_y(int order_idx) {
    return (Text::font_size * 2) * (1 + order_idx / 18);
}

void TrackerWindow::set_tempo_cb(const std::string &value) {
    int val = atoi_pos<int>(value);
    data.lock();
    if (val > 0) {
	data.ticklen = val;
    } else {
	data.ticklen = 20000; // just some default here
    }
    data.unlock();
}

void TrackerWindow::set_pattern_rows_cb(const std::string &value) {
    int val = atoi_pos<int>(value);
    data.lock();
    if (val > 0 && val <= data.max_num_rows) {
	data.num_rows = val;
    }
    data.unlock();
    cursor_move(0, 0);
}

void TrackerWindow::set_pattern_tracks_cb(const std::string &value) {
    int val = atoi_pos<int>(value);
    data.lock();
    if (val > 0 && val <= data.max_num_tracks) {
	data.num_tracks = val;
    }
    data.unlock();
    cursor_move(0, 0);
}

void TrackerWindow::set_master_hb_coeff_cb(const std::string &value) {
    float val = strtof(value.c_str(), NULL);
    data.lock();
    if (val >= 0.0 && val <= 1.0) {
	data.master_hb_coeff = val;
    }
    data.unlock();
}

void TrackerWindow::set_master_hb_mix_cb(const std::string &value) {
    float val = strtof(value.c_str(), NULL);
    data.lock();
    data.master_hb_mix = val;
    data.unlock();
}

size_t TrackerWindow::get_current_pattern() {
    return current_pattern;
}

int TrackerWindow::get_current_row() {
    return cursor_row;
}

int TrackerWindow::get_order_cursor_pos() {
    return order_cursor_pos;
}

void TrackerWindow::report_playstate(unsigned int current_pattern, unsigned int current_row) {
    playstate_mutex.lock();
    if (playing_pattern != current_pattern ||
	playing_row != current_row) {
	changed = true;
    }
    playing_pattern = current_pattern;
    playing_row = current_row;
    playstate_mutex.unlock();
}

void TrackerWindow::update_data(TrackerData &data) {
    data.master_hb_coeff = strtof(master_hb_coeff_textbox.value.c_str(), NULL);
    data.master_hb_mix = strtof(master_hb_mix_textbox.value.c_str(), NULL);
    // Update module skip flags according to track mutes
    // FIXME: currently this relies on being called only after PatchEditorWindow::update_data()
    for (auto i : irange(0, TrackerData::max_num_tracks)) {
	if (mute_flags[i]) {
	    for (auto m : irange(0, TrackerData::max_num_modules)) {
		data.module_skip_flags[i * TrackerData::max_num_modules + m] = 1;
	    }
	}
    }
}

Json::Value TrackerWindow::as_json() {
    Json::Value json(Json::objectValue);
    json["master_hb_coeff"] = master_hb_coeff_textbox.value;
    json["master_hb_mix"] = master_hb_mix_textbox.value;
    return json;
}

void TrackerWindow::from_json(Json::Value &json_) {
    auto &json = json_["_tracker"];
    if (json.isObject()) {
	master_hb_coeff_textbox.value = json["master_hb_coeff"].asString();
	master_hb_mix_textbox.value = json["master_hb_mix"].asString();
    }
}
