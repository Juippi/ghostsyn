#include "main_window.hpp"
#include "fonts.hpp"
#include <iostream>
#include <SDL.h>
#include <boost/filesystem.hpp>

int main(int argc, char *argv[]) {
    if (argc < 2) {
	std::cerr << argv[0] << " <songfile>" << std::endl;
	return 1;
    }

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

    boost::filesystem::path my_path(argv[0]);
    std::cerr << "set font dir " << my_path.parent_path() << std::endl;
    Fonts::instance().font_dir = my_path.parent_path();

    std::cerr << "editing song " << argv[1] << std::endl;

    try {
	MainWindow main_window(1600, 1000, argv[1]);
	bool quit = false;

	SDL_Event event;
	bool show = false;
	int wait_timeout = 0;

	while (!quit) {

	    while (SDL_WaitEventTimeout(&event, wait_timeout)) {
		switch (event.type) {
		case SDL_QUIT:
		    quit = true;
		    break;
		case SDL_MOUSEMOTION:
		    main_window.mouse_move(event.motion.x, event.motion.y, true);
		    break;
		case SDL_MOUSEBUTTONDOWN:
		    main_window.mouse_click(event.button.button,
					    event.button.x, event.button.y, true);
		    break;
		case SDL_MOUSEBUTTONUP:
		    main_window.mouse_release(event.button.button,
					      event.button.x, event.button.y, true);
		    break;
		case SDL_WINDOWEVENT:
		    if (event.window.event == SDL_WINDOWEVENT_SHOWN ||
			event.window.event == SDL_WINDOWEVENT_EXPOSED) {
			show = true;
			break;
		    }
		    break;
		case SDL_KEYDOWN:
		    if (event.key.keysym.mod & KMOD_CTRL && event.key.keysym.sym == SDLK_q) {
			quit = true;
		    } else {
			main_window.key_down(event.key.keysym);
		    }
		    break;
		case SDL_KEYUP:
		    main_window.key_up(event.key.keysym);
		    break;
		case SDL_TEXTEDITING:
		    std::cerr << "textediting "<< event.text.text << std::endl;
		    main_window.edit_text(event.text.text, false);
		    break;
		case SDL_TEXTINPUT:
		    std::cerr << "textinput "<< event.text.text << std::endl;
		    main_window.edit_text(event.text.text, true);
		    break;
		}
		wait_timeout = 0;
	    }

	    if (main_window.just_changed() || show) {
		main_window.update();
		show = false;
	    }
	    wait_timeout = 10;
	}

    } catch (std::string err) {
	std::cerr << "FATAL: " << err << std::endl;
    }

    return 0;
}
