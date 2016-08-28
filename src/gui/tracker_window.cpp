#include "tracker_window.hpp"
#include "utils.hpp"
#include <iostream>
#include <string>
#include <functional>
#include <boost/range/irange.hpp>

using boost::irange;

TrackerWindow::TrackerWindow(int x, int y, int width, int height, SDL_Renderer *renderer,
			     TTF_Font *text_font, ModalPromptInterface &prompt_input_,
			     TrackerData &data_)
    : EditorWindow(x, y, width, height, renderer, text_font, prompt_input_),
      data(data_),
      cursor_area(TrackerWindow::CURSOR_AREA_TRACKER),
      order_shrink("-",
		   std::bind(&TrackerWindow::order_adjust, this, -1),
		   850, (UI::Text::font_size * 2)),
      order_grow("+",
		 std::bind(&TrackerWindow::order_adjust, this, 1),
		 850 + 70, (UI::Text::font_size * 2)),
      tempo_textbox(UI::Text::char_width * 4 * 9,
		    UI::Text::font_size * 2 * 6,
		    6,
		    tostr(data.ticklen),
		    std::bind(&TrackerWindow::set_tempo_cb, this,
			      std::placeholders::_1)) {
    register_(order_shrink);
    register_(order_grow);
    register_(tempo_textbox);
}

void TrackerWindow::key_down(const SDL_Keysym &sym) {
    EditorWindow::key_down(sym);

    // no numpad
    if (cursor_area == CURSOR_AREA_TRACKER) {
	if (sym.mod & KMOD_CTRL) {
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
	    }

	    // CTRL-SHIFT-[xcv] : cut/copy/paste pattern
	    if (sym.mod & KMOD_SHIFT) {
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
		}

	    // CTRL-[xcv] : cut/copy/paste track
	    } else {
		switch(sym.sym) {
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
	    }

	} else {

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
				 ((UI::Text::font_size * 2) + UI::Tracker::row_pad) *
				 (UI::Tracker::visible_rows - UI::Tracker::row_pad));

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
	track_width = UI::Text::char_width * 2 * 14;
	hw_effects = false;
	hw_notes = false;
    } else if (data.num_tracks <= 6) {
	track_width = UI::Text::char_width * 2 * 10;
	hw_effects = true;
	hw_notes = false;
    } else {
	track_width = UI::Text::char_width * 2 * 7;
	hw_effects = true;
	hw_notes = true;
    }

    auto &patt = data.patterns[current_pattern];
    for (int disp_row = 0; disp_row < UI::Tracker::visible_rows; ++disp_row) {
	int row_idx = cursor_row - (UI::Tracker::visible_rows / 2) + disp_row;
	if (row_idx < 0 || static_cast<size_t>(row_idx) >= patt.num_rows) {
	    continue;
	}

	std::stringstream lineno;
	lineno.width(2);
	lineno.fill('0');
	lineno << std::hex << row_idx;

	SDL_Color text_color = {160, 160, 160, 255};
	draw_text_hw(4, TRACKER_START_Y + UI::Tracker::row_pad + ((UI::Text::font_size * 2) + UI::Tracker::row_pad) * disp_row,
		     lineno.str(), Color(160, 160, 160, 255));

	int track_idx = 0;
	for (auto &track : patt.tracks) {
	    Pattern::Cell &cell = track.cells[row_idx];

	    int start_x = (40 + UI::Tracker::track_pad +
			   (track_width + UI::Tracker::track_pad) * track_idx);
	    int start_y = TRACKER_START_Y + UI::Tracker::row_pad + ((UI::Text::font_size * 2) + UI::Tracker::row_pad) * disp_row;

	    Color cell_color;
	    if (row_idx == cursor_row) {
		cell_color = colors.cursor_row;
	    } else if (playing_pattern == current_pattern &&
		       playing_row == static_cast<unsigned int>(row_idx)) {
		cell_color = colors.play_cursor;
	    } else if (row_idx % 2 == 0) {
		cell_color = colors.row_2;
	    // } else if (row_idx % 24 == 0) {
	    //	cell_color = colors.row_16;
	    // } else if (row_idx % 6 == 0) {
	    //	cell_color = colors.row_4;
	    } else {
		cell_color = colors.tracker_bg;
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
	      draw_text_hw(start_x, start_y, r_text.str(), default_color_fg, cell_color);
	    } else {
		draw_text(start_x, start_y, r_text.str(), default_color_fg, cell_color);
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
		    x = start_x + UI::Text::char_width * 2 * 4;
		} else {
		    x = start_x + UI::Text::char_width * 2 * 7;
		}
		if (hw_effects) {
		    draw_text_hw(x, start_y, r_text.str(), default_color_fg, cell_color);
		} else {
		    draw_text(x, start_y, r_text.str(), default_color_fg, cell_color);
		}
	    }

	    // instrument number
	    if (cell.note > 0 || cell.octave > 0) {
		std::stringstream instr_num_text;
		instr_num_text << cell.instrument;
		int x;
		if (hw_notes) {
		    x = start_x + UI::Text::char_width * 2 * 3;
		} else {
		    x = start_x + UI::Text::char_width * 4 * 3;
		}
		draw_text_hw(x, start_y, instr_num_text.str(),
			     colors.instrument_num, cell_color);
	    }

	    // cursor
	    if (cursor_area == CURSOR_AREA_TRACKER &&
		row_idx == cursor_row &&
		cursor_x / 4 == track_idx) {
		Rect cursor_rect;
		if (cursor_x % 4 == 0) {
		    cursor_rect.x = start_x;
		    if (hw_notes) {
			cursor_rect.w = (UI::Text::char_width * 2) * 3;
		    } else {
			cursor_rect.w = (UI::Text::char_width * 4) * 3;
		    }
		} else {
		    if (hw_effects) {
			if (hw_notes) {
			    cursor_rect.x = start_x + (UI::Text::char_width * 2) * (3 + cursor_x % 4);
			} else {
			    cursor_rect.x = start_x + (UI::Text::char_width * 2) * (6 + cursor_x % 4);
			}
			cursor_rect.w = (UI::Text::char_width * 2);
		    } else {
			// TODO: ignore hw_notes here for now, at the moment can't happen
			cursor_rect.x = start_x + (UI::Text::char_width * 4) * (3 + cursor_x % 4);
			cursor_rect.w = (UI::Text::char_width * 4);
		    }
		}
		cursor_rect.y = start_y;
		cursor_rect.h = (UI::Text::font_size * 2);
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
    draw_text(0, (UI::Text::font_size * 2), o_text.str());
    int order_idx = 0;
    for (auto n : data.order) {
	o_text.clear();
	o_text.str(std::string());
	if (n < 9) {
	    o_text << " ";
	}
	o_text << (n + 1);
	draw_text_hw(order_x(order_idx), order_y(order_idx), o_text.str());
	++order_idx;
    }

    // Order cursor
    if (cursor_area == CURSOR_AREA_ORDER) {
	draw_rect(order_x(order_cursor_pos), order_y(order_cursor_pos),
		  UI::Text::char_width * 4, UI::Text::font_size * 2);
    }

    std::stringstream oct_text;
    oct_text << "Octave : " << current_octave;
    draw_text(0, (UI::Text::font_size * 2) * 4, oct_text.str());

    std::stringstream instr_text;
    instr_text << "Instr  : " << current_instrument;
    draw_text(0, (UI::Text::font_size * 2) * 5, instr_text.str());

    std::stringstream tempo_text;
    tempo_text << "Tempo  : ";
    draw_text(0, (UI::Text::font_size * 2) * 6, tempo_text.str());

    // Song length
    long long samples = data.order.size() * data.num_rows * data.ticklen;
    long long seconds = samples / (44100 * 2);
    char buf[10];
    snprintf(buf, 10, "%02lld:%02lld", seconds / 60, seconds % 60);
    draw_text(width - UI::Text::char_width * 5 * 4, 0, buf);

    data.unlock();
    EditorWindow::update();
}

void TrackerWindow::cursor_move(int x, int y) {
    Pattern &cp = data.patterns[current_pattern];
    if (x < 0) {
	cursor_x = std::max(0, cursor_x + x);
    } else {
	cursor_x = std::min(static_cast<int>(cp.num_tracks * 4) - 1, cursor_x + x);
    }
    if (y < 0) {
	cursor_row = std::max(0, cursor_row + y);
    } else {
	cursor_row = std::min(static_cast<int>(cp.num_rows) - 1, cursor_row + y);
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
    return (UI::Text::char_width * 4) / 2 * (16 + (order_idx % 18) * 2);
}

int TrackerWindow::order_y(int order_idx) {
    return (UI::Text::font_size * 2) * (1 + order_idx / 18);
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

size_t TrackerWindow::get_current_pattern() {
    return current_pattern;
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
