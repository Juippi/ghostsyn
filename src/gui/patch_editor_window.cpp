#include "patch_editor_window.hpp"
#include "utils.hpp"
#include <string>
#include <functional>
#include <iostream>
#include <stack>
#include <algorithm>
#include <list>
#include <cstdlib>
#include <boost/range/irange.hpp>

using boost::irange;

int PatchEditorWindow::EditorModule::hovered_input_impl(int px, int py) {
    // N params + 1 trigger field + stereo toggle + shape selection
    int top = y + UI::Text::char_height * 2 + UI::Text::row_gap;
    int hovered = -1;
    if (px >= x && px < x + width &&
	py >= top) {
	hovered = (py - top) / (UI::Text::char_height + UI::Text::row_gap);
    }
    return hovered;
}

PatchEditorWindow::EditorModule::~EditorModule() {
}

int PatchEditorWindow::EditorModule::hovered_param(int px, int py) {
    int no_inputs = (module.type == Module::ModuleType::TYPE_OSC ? 3 : 1);
    int param = hovered_input_impl(px, py) - no_inputs;
    if (param >= 0 && param < static_cast<int>(string_values.size())) {
	return param;
    } else {
	return -1;
    }
}

int PatchEditorWindow::EditorModule::hovered_input(int px, int py) {
    int no_inputs = (module.type == Module::ModuleType::TYPE_OSC ? 3 : 1);
    int input = hovered_input_impl(px, py);
    if (input >= 0 && input < no_inputs) {
	return input;
    } else {
	return -1;
    }
}

void PatchEditorWindow::EditorModule::adjust_size() {
    height = ((2 + 3 + module.params.size()) *
	      (UI::Text::char_height + UI::Text::row_gap) +
	      2); // pad (todo: to ui constants)
    int widest = PatchEditorWindow::MODULE_SIZE;
    for (auto &val : string_values) {
	int w = UI::Text::char_width * (5 + val.size()) + 2;
	if (w > widest) {
	    widest = w;
	}
    }
    width = widest;
}

bool PatchEditorWindow::EditorModule::is_inside(int px, int py) {
    return (px >= x && px <= (x + width) &&
	    py >= y && py <= (y + height));
}

void PatchEditorWindow::EditorModule::draw(int x, int y) {
}

void PatchEditorWindow::clamp_module_pos(EditorModule &module) {
    module.x = std::min(std::max(module.x, MODULE_AREA_PADDING),
			 width - (MODULE_SIZE + MODULE_AREA_PADDING));
    module.y = std::min(std::max(module.y, MODULE_AREA_PADDING),
			 height - (MODULE_SIZE + MODULE_AREA_PADDING));
}

bool PatchEditorWindow::has_connection_cycle() {
    bool has_cycle = false;
    for (size_t start : irange(0u, modules.size())) {
	int idx = start;
	std::set<int> visited;
	do {
	    if (visited.find(idx) != visited.end()) {
		has_cycle = true;
		break;
	    }
	    visited.insert(idx);
	} while ((idx = modules[idx].module.out_module) >= 0);
	if (has_cycle) {
	    break;
	}
    }
    return has_cycle;
}

void PatchEditorWindow::update_params_from_ui() {
    for (size_t i : irange(0u, modules.size())) {
	auto &e = modules[i];
	for (size_t k : irange(0u, std::min(e.string_values.size(), e.module.params.size()))) {
	    const auto &str_val = e.string_values[k];
	    float float_val = strtof(str_val.c_str(), NULL);
	    e.module.params[k].float_value = float_val;
	    std::cerr << "set: module " << i << " param " << k << " value " << float_val << std::endl;
	}
    }
}

void PatchEditorWindow::update_track_instruments_from_ui() {
    for (size_t i : irange(0u, track_instruments.size())) {
	track_instruments[i].first = atoi_pos(instr_textboxes[i * 2].value.c_str(),
					      0u, instrument_triggers.size());
	track_instruments[i].second = atoi_pos(instr_textboxes[i * 2 + 1].value.c_str(),
					       0u, instrument_triggers.size());
    }
}

void PatchEditorWindow::change_page(int dir) {
    int new_shown_page = std::min(std::max(0, shown_page + dir), num_pages - 1);
    if (new_shown_page != shown_page) {
	shown_page = new_shown_page;
	selected_module = hovered_module = hovered_param = -1;
	changed = true;
    }
}

PatchEditorWindow::PatchEditorWindow(int x, int y, int width, int height, int num_tracks,
				     SDL_Renderer *renderer, TTF_Font *font,
				     ModalPromptInterface &prompt_input_)
    : EditorWindow(x, y, width, height, renderer, font, prompt_input_),
      prev_page("Prev",
		std::bind(&PatchEditorWindow::change_page, this, -1),
		10, (Button::default_height + 8) * 6),
      next_page("Next",
		std::bind(&PatchEditorWindow::change_page, this, 1),
		10 + UI::Text::char_width * 2 * 5, (Button::default_height + 8) * 6)
{
    // master mix out, immovable && indestructible
    auto master_out = EditorModule();
    master_out.label = "out";
    master_out.x = width / 2;
    master_out.y = height - 100;
    master_out.is_master_out = true;
    modules.push_back(master_out);

    register_(prev_page);
    register_(next_page);

    action_buttons.push_back(Button("New osc",
				    std::bind(&PatchEditorWindow::new_module, this, Module::TYPE_OSC),
				    10, 10, 162));
    action_buttons.push_back(Button("New flt",
				    std::bind(&PatchEditorWindow::new_module, this, Module::TYPE_FILTER),
				    10, 10 + Button::default_height + 8, 162));
    action_buttons.push_back(Button("New env",
				    std::bind(&PatchEditorWindow::new_module, this, Module::TYPE_ENV),
				    10, 10 + (Button::default_height + 8) * 2, 162));
    action_buttons.push_back(Button("New comp",
				    std::bind(&PatchEditorWindow::new_module, this, Module::TYPE_COMP),
				    10 + 162 + 8, 10, 162));
    action_buttons.push_back(Button("Del module",
				    std::bind(&PatchEditorWindow::del_module, this),
				    10 + (162 + 8) * 2, 10, 162));
    for (int i : irange(0, num_tracks)) {
	int tx = 20 + UI::Text::char_width * 11 * (i % 4);
	int ty = 10 + (Button::default_height + 8) * (3 + i / 4);
	instr_textboxes.push_back(TextBox(tx, ty, 2, "0"));
	instr_textboxes.push_back(TextBox(tx + UI::Text::char_width * 5, ty, 2, "0"));
    }

    for (auto &button : action_buttons) {
	register_(button);
    }
    for (auto &textbox : instr_textboxes) {
	register_(textbox);
    }

    instrument_triggers.resize(num_tracks);
    for (int i : irange(0, num_tracks)) {
	track_instruments.push_back(std::pair<int, int>(0, 0));
    }
    for (int i : irange(0, PatchEditorWindow::NUM_INSTRUMENTS)) {
	instrument_triggers.push_back(std::pair<int, int>(0, 0));
    }
}

PatchEditorWindow::~PatchEditorWindow() {
}

void PatchEditorWindow::from_json(Json::Value &json_) {
    auto &json = json_["_patch_editor"];
    modules.clear();
    for (auto &module_json : json["modules"]) {
	EditorModule module;
	module.label = module_json["label"].asString();
	module.x = module_json["x"].asInt();
	module.y = module_json["y"].asInt();
	module.width = module_json["width"].asInt();
	module.height = module_json["height"].asInt();
	module.page = module_json["page"].asInt();
	module.is_master_out = module_json["is_master_out"].asBool();
	module.module.from_json(module_json["module"], 0);
	module.string_trigger = module_json["string_trigger"].asString();
	module.trigger_instrument = module_json["trigger_instrument"].asInt();
	for (auto &string_value : module_json["string_values"]) {
	    module.string_values.push_back(string_value.asString());
	}
	size_t button_idx = 0;
	for (auto &instrument : module_json["track_instruments"]) {
	    instr_textboxes[button_idx++].value = instrument.asString();
	}
	clamp_module_pos(module);
	module.adjust_size();
	modules.push_back(module);
    }
    hovered_module = -1;
    selected_module = -1;
    dragging = false;
}

Json::Value PatchEditorWindow::as_json() {
    Json::Value json(Json::objectValue);
    Json::Value modules_json(Json::arrayValue);
    for (auto &module : modules) {
	Json::Value module_json(Json::objectValue);
	module_json["label"] = module.label;
	module_json["x"] = module.x;
	module_json["y"] = module.y;
	module_json["width"] = module.width;
	module_json["height"] = module.height;
	module_json["page"] = module.page;
	module_json["is_master_out"] = module.is_master_out;
	module_json["module"] = module.module.as_json();
	module_json["string_trigger"] = module.string_trigger;
	module_json["trigger_instrument"] = module.trigger_instrument;
	Json::Value string_values_json(Json::arrayValue);
	for (auto &string_value : module.string_values) {
	    string_values_json.append(string_value);
	}
	Json::Value track_instruments_json(Json::arrayValue);
	module_json["string_values"] = string_values_json;
	for (auto &textbox : instr_textboxes) {
	    track_instruments_json.append(textbox.value);
	}
	module_json["track_instruments"] = track_instruments_json;
	modules_json.append(module_json);
    }
    json["modules"] = modules_json;
    return json;
}

void PatchEditorWindow::mouse_move(int x, int y, bool inside) {
    if (inside) {
	if (dragging) {
	    modules[selected_module].x = x + drag_offset_x;
	    modules[selected_module].y = y + drag_offset_y;
	    clamp_module_pos(modules[selected_module]);
	    changed = true;
	} else {
	    for (size_t i : irange(0u, modules.size())) {
		auto &module = modules[i];

		if (!module.is_master_out && module.page != shown_page) {
		    continue;
		}

		if (module.is_inside(x, y)) {
		    int new_hovered_param = module.hovered_param(x, y);
		    if (new_hovered_param != hovered_param) {
			hovered_param = new_hovered_param;
			changed = true;
		    }
		    int new_hovered_input = module.hovered_input(x, y);
		    if (new_hovered_input != hovered_input) {
			hovered_input = new_hovered_input;
			changed = true;
		    }
		    if (hovered_module != static_cast<int>(i)) {
			hovered_module = static_cast<int>(i);
			hovered_param = module.hovered_param(x, y);
			hovered_input = module.hovered_input(x, y);
			changed = true;
		    }
		    return;
		}
	    }
	    if (hovered_module != -1) {
		hovered_module = -1;
		changed = true;
	    }
	}
    }
    EditorWindow::mouse_move(x, y, inside);
}

void PatchEditorWindow::mouse_click(int button_idx, int x, int y, bool inside) {
    if (inside) {
	switch (button_idx) {
	case SDL_BUTTON_LEFT:
	    if (hovered_module >= 0 &&
		selected_module != hovered_module &&
		!modules[hovered_module].is_master_out) {
		selected_module = hovered_module;
		changed = true;
	    }
	    if (selected_module == hovered_module &&
		!modules[selected_module].is_master_out) {
		dragging = true;
		drag_offset_x = modules[selected_module].x - x;
		drag_offset_y = modules[selected_module].y - y;
	    }
	    break;
	case SDL_BUTTON_RIGHT:
	    if (selected_module >= 0 && hovered_module >= 0 &&
		selected_module != hovered_module) {

		if (modules[hovered_module].is_master_out) {
		    modules[selected_module].module.out_module = hovered_module;
		    modules[selected_module].module.out_param = 0;
		    changed = true;

		} else if (hovered_param >= 0) {
		    if (modules[selected_module].module.out_module != hovered_module) {
			// std::cerr << "connect: elem " << selected_module
			//	  << " -> " << hovered_module << ":" << hovered_param << std::endl;
			modules[selected_module].module.out_module = hovered_module;
			// Synth parameters start after type and out offset words at idx 2
			modules[selected_module].module.out_param = hovered_param + 2;
		    } else {
			modules[selected_module].module.out_module = -1;
			modules[selected_module].module.out_param = -1;
		    }
		    changed = true;
		}
	    }
	    break;
	case SDL_BUTTON_MIDDLE:
	{
	    if (hovered_module >= 0) {
		auto &he = modules[hovered_module];
		// TODO: ensure we abort prompt if any changes to modules made
		if (hovered_param >= 0) {
		    std::string &val = modules[hovered_module].string_values[hovered_param];
		    prompt_input.prompt_input("Enter param value",
					      val,
					      std::bind(&EditorWindow::str_assign, std::ref(val), std::placeholders::_1));
		} else if (hovered_input == 0) {
		    std::string &val = he.string_trigger;
		    prompt_input.prompt_input("Enter value",
					      val,
					      std::bind(&EditorWindow::str_assign, std::ref(val), std::placeholders::_1));
		} else if (hovered_input == 1) {
		    he.module.stereo = !he.module.stereo;
		    changed = true;
		} else if (hovered_input == 2) {
		    he.module.osc_shape =
			static_cast<Module::OscShape>((he.module.osc_shape + 1) % Module::OscShape::OSC_NUM_SHAPES);
		    changed = true;
		}
	    }
	    break;
	}
	}
    }
    EditorWindow::mouse_click(button_idx, x, y, inside);
}

void PatchEditorWindow::mouse_release(int button_idx, int x, int y, bool inside) {
    switch (button_idx) {
    case SDL_BUTTON_LEFT:
	dragging = false;
	break;
    }
    EditorWindow::mouse_release(button_idx, x, y, inside);
}

void PatchEditorWindow::key_down(const SDL_Keysym &sym) {
    EditorWindow::key_down(sym);
}

void PatchEditorWindow::key_up(const SDL_Keysym &sym) {
    EditorWindow::key_up(sym);
}

void PatchEditorWindow::draw_module(const EditorModule &module,
				     const Color &edge_color,
				     bool is_hovered,
				     int idx) {
    draw_rect(module.x, module.y,
	      module.width, module.height,
	      default_color_bg, true);
    draw_rect(module.x, module.y,
	      module.width, module.height, edge_color);
    draw_text_hw(module.x + 1, module.y + 1, module.label);
    std::stringstream idx_str;
    idx_str << idx;
    draw_text_small(module.x + module.width - UI::Text::char_width * idx_str.str().size() - 3,
		    module.y + 1,
		    idx_str.str());

    if (module.is_master_out) {
	draw_line(module.x + module.width / 2, module.y,
		  module.x + module.width / 2, module.y - 16);
	draw_rect(module.x + module.width / 2 - 2, module.y - 16 - 2,
		  4, 4);
    } else {
	draw_line(module.x + module.width / 2, module.y + module.height,
		  module.x + module.width / 2, module.y + module.height + 16);
	draw_rect(module.x + module.width / 2 - 2, module.y + module.height + 16 - 2,
		  4, 4);
    }

    int y_off = 2 * UI::Text::char_height + UI::Text::row_gap;
    draw_text_small(module.x + 1, module.y + y_off, module.string_trigger);
    y_off += UI::Text::char_height + UI::Text::row_gap;

    if (!module.is_master_out) {
	if (module.module.type == Module::ModuleType::TYPE_OSC) {
	    // Stereo checkbox
	    draw_text_small(module.x, module.y + y_off,
			    "ster");
	    int cbox_x = module.x + 1 + UI::Text::char_width * 5;
	    int cbox_y = module.y + y_off;
	    int cbox_w = UI::Text::char_width * 2;
	    int cbox_h = UI::Text::char_height;
	    draw_rect(cbox_x, cbox_y, cbox_w, cbox_h);
	    if (module.module.stereo) {
		draw_line(cbox_x + 2, cbox_y + 2, cbox_x + cbox_w - 2, cbox_y + cbox_h - 2);
		draw_line(cbox_x + 2, cbox_y + cbox_h - 2, cbox_x + cbox_w - 2, cbox_y + 2);
	    }
	    y_off += (UI::Text::char_height + UI::Text::row_gap);

	    // Shape selector
	    draw_text_small(module.x, module.y + y_off,
			    module.module.osc_shape_names.at(module.module.osc_shape));
	    y_off += (UI::Text::char_height + UI::Text::row_gap);
	}
	// Parameters w/ inputs
	for (auto i : irange(0u, module.module.params.size())) {
	    const Module::Param &param = module.module.params[i];
	    // Parameter connection point
	    draw_line(module.x, module.y + y_off + UI::Text::char_height / 2,
		      module.x - 16, module.y + y_off + UI::Text::char_height / 2);
	    draw_rect(module.x - 16 - 2, module.y + y_off + UI::Text::char_height / 2 - 2,
		      4, 4);
	    // Parameter label & value
	    draw_text_small(module.x + 1, module.y + y_off,
			    module_param_labels[module.module.type][i]);
	    draw_text_small(module.x + 1 + UI::Text::char_width * 5,
			    module.y + y_off, module.string_values[i]);
	    if (is_hovered && hovered_param == static_cast<int>(i)) {
		draw_rect(module.x + 1, module.y + y_off,
			  module.width, UI::Text::char_height);
	    }
	    y_off += (UI::Text::char_height + UI::Text::row_gap);
	}
    }
}

void PatchEditorWindow::update() {
    draw_rect(2, 2, width - 4, height - 4);

    // Connections
    for (auto &module : modules) {
	if (module.page != shown_page) {
	    continue;
	}

	if (module.module.out_module >= 0 &&
	    static_cast<size_t>(module.module.out_module) < modules.size() &&
	    module.module.out_param >= 0) {
	    auto &module2 = modules[module.module.out_module];
	    int y_off;
	    if (module2.is_master_out) {
		draw_line(module.x + module.width / 2,
			  module.y + module.height + 16,
			  module2.x + module2.width / 2,
			  module2.y - 16);
	    } else {
		int skip = (module2.module.type == Module::ModuleType::TYPE_OSC ? 3 : 1);
		int y_off = UI::Text::char_height / 2 +
		    ((module.module.out_param + skip) *
		     (UI::Text::char_height + UI::Text::row_gap));
		draw_line(module.x + module.width / 2,
			  module.y + module.height + 16,
			  module2.x - 16,
			  module2.y + y_off);
	    }
	}
    }

    // Modules
    for (size_t i : irange(0u, modules.size())) {
	auto &module = modules[i];

	if (module.is_master_out || module.page == shown_page) {
	    auto idx = static_cast<int>(i);

	    const EditorModule &e = module;
	    bool is_hovered = (hovered_module == static_cast<int>(i));
	    if (idx == selected_module) {
		draw_module(e, selected_color, is_hovered, i);
	    } else if (idx == hovered_module) {
		draw_module(e, hovered_color, is_hovered, i);
	    } else {
		draw_module(e, default_color_fg, is_hovered, i);
	    }
	}
    }

    std::stringstream page_str;
    page_str << shown_page;
    draw_text(width - UI::Text::char_width * 8,
	      (Button::default_height + 8) * 6,
	      page_str.str());

    EditorWindow::update();
}

void PatchEditorWindow::new_module(Module::ModuleType type) {
    // std::cerr << "new module" << std::endl;
    EditorModule m = EditorModule();
    m.label = module_disp_types[type];
    m.x = 100;
    m.y = 500;
    m.module = Module(type);
    m.page = shown_page;
    m.string_values.resize(m.module.params.size());
    for (auto &v : m.string_values) {
	v = "0.0";
    }
    m.adjust_size();
    modules.push_back(m);
    changed = true;
}

void PatchEditorWindow::del_module() {
    // std::cerr << "del module " << selected_module << std::endl;
    if (selected_module >= 0 &&
	!modules[selected_module].is_master_out) {
	modules.erase(modules.begin() + selected_module);
	for (auto &module : modules) {
	    if (module.module.out_module >= selected_module) {
		--module.module.out_module;
	    }
	}
	hovered_module = -1;
	selected_module = -1;
	dragging = false;
	changed = true;
    }
}

void PatchEditorWindow::convert_instruments(TrackerData &data) {
    // std::cerr << "*** convert_instruments, has " << modules.size() << " modules" << std::endl;

    if (has_connection_cycle()) {
	std::cerr << "Can't convert cyclical conn. graph to synth instruments!" << std::endl;
	return;
    }
    // std::cerr << "Cycle check OK, converting instruments for synth" << std::endl;

    data.modules.clear();
    data.trigger_points.clear();

    update_params_from_ui();
    update_track_instruments_from_ui();

    // Determine which modules actually need to run on which tracks.
    // Iterate through modules, for each which is triggered by track's primary
    // or alternate instrument, recursivery mark all connected modules
    for (auto tidx : irange(0u, data.num_tracks)) {
	std::set<int> needed_so_far;
	for (int eidx : irange(0u, modules.size())) {

	    int e = eidx;
	    bool needed = false;
	    std::set<int> traversed;
	    do {
		traversed.insert(e);
		int ti = atoi_pos<int>(modules[eidx].string_trigger);
		if (ti >= 0 &&
		    (track_instruments[tidx].first == ti ||
		     track_instruments[tidx].second == ti)) {
		    needed = true;
		} else if (needed_so_far.find(e) != needed_so_far.end()) {
		    needed = true;
		}
	    } while ((e = modules[e].module.out_module) > 0);

	    if (needed) {
		for (int needed_e : traversed) {
		    std::cerr << "track " << tidx << " needs " << needed_e << std::endl;
		    needed_so_far.insert(needed_e);
		}
	    }
	}
	for (int needed_e : needed_so_far) {
	    modules[needed_e].using_tracks.insert(tidx);
	}
	std::cerr << std::endl;
    }

    // Sort algo: start for master out, work up the tree (depth or breadth first),
    // this will also exclude unconnected modules automatically
    std::vector<Module> dump_modules;
    std::vector<int> dump_order;
    std::stack<int> bfs_stack;
    bfs_stack.push(0); // start at master out

    while (bfs_stack.size() > 0) {
	int current = bfs_stack.top();
	bfs_stack.pop();

	// std::cerr << "bfs dump current: " << current << std::endl;

	for (size_t i : irange(0u, modules.size())) {
	    if (modules[i].module.out_module == current) {
		bfs_stack.push(i);
	    }
	}

	if (!modules[current].is_master_out) {
	    // std::cerr << "dump: " << current << " ("
	    //	      << modules[current].label << ")" << std::endl;
	    dump_modules.push_back(modules[current].module);
	    dump_order.push_back(current);
	}
    }

    // We traversed graph from master out outwards, so actual dump order
    // should be reverse.
    // Connection operators: first connection to a particular destination should
    // have operator OP_SET, subsequent ones OP_ADD.
    std::reverse(dump_order.begin(), dump_order.end());
    std::reverse(dump_modules.begin(), dump_modules.end());

    const int max_num_modules = 32; // fixme: must match synth NUM_MODULES constant
    data.module_skip_flags.clear();
    data.module_skip_flags.resize(data.num_tracks * max_num_modules);
    for (auto &v : data.module_skip_flags) {
	v = 1;
    }

    std::cerr << "skip flag array:" << std::endl;
    for (int tidx : irange(0u, data.num_tracks)) {
	for (int oidx : irange(0u, dump_order.size())) {
	    auto &e = modules[dump_order[oidx]];
	    if (e.using_tracks.find(tidx) != e.using_tracks.end()) {
		data.module_skip_flags[tidx * max_num_modules + oidx] = 0;
	    }
	    std::cerr << " " << (data.module_skip_flags[tidx * max_num_modules + oidx] ? "1" : "0");
	}
	std::cerr << std::endl;
    }

    // Keep track of all module-param pairs to which we've already seen a connection.
    std::set<std::string> all_outs;

    for (size_t i : irange(0u, dump_modules.size())) {
	Module m = dump_modules[i]; // must copy, we modify module output index
	int mapped_out = -1;
	if (m.out_module != 0) {
	    for (mapped_out = 0; mapped_out < static_cast<int>(dump_order.size()); ++mapped_out) {
		if (dump_order[mapped_out] == m.out_module) {
		    break;
		}
	    }
	}
	// std::cerr << "mapped out " << e.out_module << " to " << mapped_out << std::endl;
	m.out_module = mapped_out;
	m.my_index = i;

	std::stringstream out_key;
	out_key << m.out_module << "_" << m.out_param;

	// TODO: if destination is master out, operation has no effect so shoud always
	// set to OP_SET (zero byte) for likely better compression
	if (all_outs.find(out_key.str()) == all_outs.end()) {
	    all_outs.insert(out_key.str());
	    // first module outputting to this destination, needs to overwrite prev. value
	    m.out_op = Module::OP_SET;
	    // std::cerr << " op: set" << std::endl;
	} else {
	    // sum any subsequent signals to the first one
	    m.out_op = Module::OP_ADD;
	    // std::cerr << " op: add" << std::endl;
	}
	data.modules.push_back(m);
    }

    std::cerr << "dumped modules in order:";
    for (auto i : dump_order) {
	std::cerr << " " << i;
    }
    std::cerr << std::endl;

    // Dump primary & secondary instrument trigger config for all tracks
    for (size_t track_idx : irange(0u, track_instruments.size())) {
	auto &track_instrument = track_instruments[track_idx];

	// std::cerr << "* track "  << track_idx << " instruments: "
	//	  << track_instrument.first << " " << track_instrument.second << std::endl;

	std::vector<int> iv = {track_instrument.first, track_instrument.second};
	for (auto instrument : iv) {
	    // std::cerr << "- instr "  << instrument << std::endl;
	    if (instrument < 0) {
		continue;
	    }

	    int triggers_left = 4;
	    for (size_t module_idx = 0;
		 module_idx < modules.size() && triggers_left > 0;
		 ++module_idx) {
		auto &module = modules[module_idx];
		int module_trigger_instr = atoi_pos<int>(module.string_trigger);

		// std::cerr << std::endl << "module " << module_idx << " triggered by instr "
		//	  << module_trigger_instr << std::endl;

		if (module_trigger_instr == static_cast<int>(instrument)) {
		    TrackerData::Trigger trig;

		    // We have index in unsorted module array, find out where the module
		    // is in dump order
		    int mapped_idx = -1;
		    for (size_t di : irange(0u, dump_order.size())) {
			if (dump_order[di] == static_cast<int>(module_idx)) {
			    mapped_idx = di;
			    break;
			}
		    }
		    if (mapped_idx < 0) {
			std::cerr << "uguu" << std::endl;
			continue;
		    }
		    trig.trig_module_idx = mapped_idx;

		    if (module.module.type == Module::ModuleType::TYPE_OSC) {
			// std::cerr << " osc trigger: set pitch of " << mapped_idx << std::endl;
			trig.trig_type = TrackerData::Trigger::SET_PITCH;
			data.trigger_points.push_back(trig);
			--triggers_left;
		    } else if (module.module.type == Module::ModuleType::TYPE_ENV) {
			// std::cerr << " env trigger: zero stage of " << mapped_idx << std::endl;
			trig.trig_type = TrackerData::Trigger::SET_ZERO;
			data.trigger_points.push_back(trig);
			--triggers_left;
		    }
		}
	    }

	    for (int i : irange(0, triggers_left)) {
		// std::cerr << " null trigger: do nothing" << std::endl;
		data.trigger_points.push_back(TrackerData::Trigger()); // null trigger
	    }
	}
    }
}
