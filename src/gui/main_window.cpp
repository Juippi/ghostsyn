#include "main_window.hpp"
#include "fonts.hpp"
#include <GL/gl.h>
#include <SDL_opengl.h>
#include <sstream>
#include <iostream>
#include <ios>

void MainWindow::save_song() {
    data.lock();

    patch_editor->update_data(data);
    tracker->update_data(data);
    Json::Value song_json = data.as_json();

    song_json["_patch_editor"] = patch_editor->as_json();
    song_json["_tracker"] = tracker->as_json();

    auto bin = data.bin();
    std::stringstream asm_str;

    asm_str << std::fixed;
    asm_str << "master_hb_c1:" << std::endl <<
	"dd " << (1 - data.master_hb_coeff) << std::endl;
    asm_str << "master_hb_c2:" << std::endl <<
	"dd " << data.master_hb_coeff << std::endl;
    asm_str << "master_hb_mix:" << std::endl <<
	"dd " << data.master_hb_mix << std::endl;

    data.unlock();

    for (auto *section : bin) {
	asm_str << section->str();
    }
    song_json["_asm"] = asm_str.str();

    std::stringstream def_str;
    // Essential definitions for compile
    def_str << "%define NUM_MODULES " << data.modules.size() << std::endl;
    def_str << "%define SONG_TICKLEN " << data.ticklen << std::endl;
    def_str << "%define NUM_ROWS " << data.num_rows << std::endl;

    def_str << "%define ENABLE_SILENCE" << std::endl;
    for (auto &nt : Module::name_types) {
	if (std::find_if(data.modules.begin(), data.modules.end(),
			 [&nt] (auto &e) -> bool { return e.type == nt.second; })
	    != data.modules.end()) {
	    std::string n = nt.first;
	    std::transform(n.begin(), n.end(), n.begin(), ::toupper);
	    def_str << "%define ENABLE_" << n << std::endl;
	}
    }
    // Does HP filter need to be enabled?
    if (std::find_if(data.modules.begin(), data.modules.end(),
                     [] (auto &m) -> bool { return (m.type == Module::ModuleType::TYPE_FILTER &&
                                                    m.filter_type == Module::FilterType::FLT_HP); })
        != data.modules.end()) {
        def_str << "%define ENABLE_FILTER_MODE_HP" << std::endl;
    }

    song_json["_defines"] = def_str.str();

    std::ofstream outfile(song_filename);
    outfile << song_json;
}

MainWindow::MainWindow(int width, int height, const std::string &song_filename_)
    : EditorWindow(0, 0, width, height, NULL, NULL, *this),
      song_filename(song_filename_), audio(data, *this),
      value_input(width / 2, height / 2) {

    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    window = SDL_CreateWindow("window",
			      0, 0,
			      width, height,
			      SDL_WINDOW_OPENGL);
    if (!window) {
	throw std::string("Failed to create window");
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
	throw std::string("Failed to create renderer for window");
    }

    gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
	throw std::string("Failed to create OpenGL context");
    }

    text_font = Fonts::instance().load_font("font.ttf", UI::Text::font_size);
    if (!text_font) {
	std::stringstream err;
	err << "Failed to load font: " << TTF_GetError();
	throw err.str();
    }

    SDL_GL_SetSwapInterval(1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    // Set up GL options for anti-aliased 2D vector graphics:
    // smooth lines & polys, no depth test
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.5, width + 0.5,
	    height + 0.5, 0.5,
	    -3.0, 3.0);
    glMatrixMode(GL_MODELVIEW);
    glClearColor(0.0, 0.0, 0.0, 0.0);

    glFrontFace(GL_CW); // Clockwise triangle strips

    std::ifstream infile(song_filename);
    if (infile.good()) {
	Json::Value song_json;
	infile >> song_json;
	data.from_json(song_json);
	patch_editor = new PatchEditorWindow(1024, 0, width - 1024, height, data.num_tracks,
					 renderer, text_font, *this);
	tracker = new TrackerWindow(0, 0, 1024, height, renderer, text_font, *this, data);
	patch_editor->from_json(song_json);
	tracker->from_json(song_json);
    } else {
	// Create initial empty song
	data.num_tracks = UI::Tracker::default_num_tracks;
	data.num_rows = UI::Tracker::default_num_rows;
	data.patterns.push_back(Pattern(UI::Tracker::default_num_tracks,
					UI::Tracker::default_num_rows));
	data.order.push_back(0);
	patch_editor = new PatchEditorWindow(1024, 0, width - 1024, height, data.num_tracks,
					 renderer, text_font, *this);
	tracker = new TrackerWindow(0, 0, 1024, height, renderer, text_font, *this, data);
    }
    children.push_back(tracker);
    children.push_back(patch_editor);
}

MainWindow::~MainWindow() {
    SDL_GL_DeleteContext(gl_context);
    // TODO: rest
}

void MainWindow::key_down(const SDL_Keysym &sym) {
    // If we have a modal prompt active, it captures all keyboard input
    // as TEXTINPUT events. Otherwise, handle key or pass on to children.
    if (prompt_active) {
	switch (sym.sym) {
	case SDLK_RETURN:
	case SDLK_KP_ENTER:
	    std::cerr << "stop text input" << std::endl;
	    SDL_StopTextInput();
	    prompt_callback(prompt_current);
	    prompt_active = false;
	    changed = true;
	    break;
	case SDLK_BACKSPACE:
	case SDLK_KP_BACKSPACE:
	    if (prompt_current.size() > 0) {
		prompt_current.resize(prompt_current.size() - 1);
		prompt_callback(prompt_current);
		changed = true;
	    }
	    break;
	}
    } else if (sym.mod & KMOD_CTRL) {
	switch(sym.sym) {
	case SDLK_s:
	    save_song();
	    std::cerr << "song saved" << std::endl;
	    break;
	default:
	    EditorWindow::key_down(sym);
	}
    } else {
	switch (sym.sym) {
	case SDLK_F5:
	    // TODO: acquire mutex before modifying data!
	    patch_editor->update_data(data);
	    tracker->update_data(data);
	    audio.play_song(0);
	    break;
	case SDLK_F6:
	    // TODO: acquire mutex before modifying data!
	    patch_editor->update_data(data);
	    tracker->update_data(data);
	    audio.play_pattern(tracker->get_current_pattern(),
			       tracker->get_current_row());
	    break;
	case SDLK_F7:
	    patch_editor->update_data(data);
	    tracker->update_data(data);
	    audio.play_song(tracker->get_order_cursor_pos());
	    break;
	case SDLK_F8:
	    audio.stop();
	    break;
	default:
	    EditorWindow::key_down(sym);
	}
    }
}

void MainWindow::edit_text(const std::string &input, bool finished) {
    if (prompt_active) {
	prompt_current += input;
	prompt_callback(prompt_current);
	if (finished) {
	    std::cerr << "stop text input" << std::endl;
	    changed = true;
	}
    }
}

void MainWindow::update() {
    // std::cerr << "update main" << std::endl;
    glClear(GL_COLOR_BUFFER_BIT);
    for (auto &child : children) {
	child->update();
    }
    EditorWindow::update();
    if (prompt_active) {
	draw_modal_input(value_input.x, value_input.y, prompt_label, prompt_current);
    }
    SDL_GL_SwapWindow(window);
}

void MainWindow::report_playstate(unsigned int current_pattern,
				  unsigned int current_row) {
    tracker->report_playstate(current_pattern, current_row);
}

void MainWindow::prompt_input(const std::string &label,
			      const std::string &initial,
			      std::function<void(std::string)> callback) {
    std::cerr << "start text input label: \"" << label << "\" initial \""
	      << initial << "\"" << std::endl;
    SDL_StartTextInput();
    prompt_active = true;
    prompt_label = label;
    prompt_orig = prompt_current = initial;
    prompt_callback = callback;
    changed = true;
}

void MainWindow::cancel_prompt() {
    if (prompt_active) {
	prompt_callback(prompt_orig);
	prompt_active = false;
	changed = true;
    }
}
