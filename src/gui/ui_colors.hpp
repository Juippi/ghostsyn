#pragma once

#include "types.hpp"

namespace UI {
    namespace Colors {
	const Color default_color_fg(255, 255, 255, 0);
	const Color default_color_bg(0, 0, 0, 0);

	const Color tracker_rownum(160, 160, 160, 255);
	const Color tracker_text(240, 240, 240, 255);
	const Color tracker_cursor_row(80, 80, 30, 255);
	const Color tracker_row_hilight1(40, 40, 20, 255);
	const Color tracker_bg(20, 20, 0, 255);
	const Color tracker_play_cursor(160, 100, 20, 255);
	const Color tracker_instrument_num(130, 130, 130, 255);
	const Color tracker_order_bg_1(50, 50, 90, 255);
	const Color tracker_order_bg_2(20, 20, 50, 255);

	const Color module_text(240, 240, 240, 255);
	const Color module_bg(40, 40, 60, 255);
	const Color module_hovered_color(0, 127, 127, 127);
	const Color module_selected_color(127, 63, 0, 127);
    }
}
