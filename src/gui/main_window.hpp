#pragma once

#include "editor_window.hpp"
#include "tracker_window.hpp"
#include "patch_editor_window.hpp"
#include "tracker_data.hpp"
#include "audio.hpp"
#include <string>

/**
   The top-level main window of editor. Owns the window, GL context & synth with data
 */

class MainWindow : public EditorWindow,
		   public PlaystateCBInterface,
		   public ModalPromptInterface {
  protected:
    SDL_Window *window = 0;
    SDL_GLContext gl_context = 0;

    std::string song_filename;
    TrackerData data;
    Audio audio;

    TrackerWindow *tracker;
    PatchEditorWindow *patch_editor;

    // Modal dialog for value inputs
    ModalInput value_input;
    bool prompt_active = false;
    std::string prompt_label;
    std::function<void(std::string)> prompt_callback;
    std::string prompt_current;
    std::string prompt_orig;

    void save_song();

  public:
    MainWindow(int width, int height, const std::string &song_filename);
    ~MainWindow();

    void key_down(const SDL_Keysym &sym) override;
    void edit_text(const std::string &input, bool finished) override;
    void update();

    // Implements PlaystateCBInterface
    void report_playstate(unsigned int current_pattern,
			  unsigned int current_row) override;

    // Implemenets ModalPromptInterface
    void prompt_input(const std::string &label,
		      const std::string &initial,
		      std::function<void(std::string)> callback) override;
    void cancel_prompt();
};
