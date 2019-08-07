#include "editor_window.hpp"
#include "tracker_data.hpp"
#include <mutex>
#include <optional>

class TrackerWindow : public EditorWindow {
protected:
    // ref to global tracker data
    TrackerData &data;
    // for copy/paste
    std::optional<Pattern::Track> copied_track;
    std::optional<Pattern> copied_pattern;

    enum {
	CURSOR_AREA_TRACKER,
	CURSOR_AREA_ORDER
    } cursor_area;
    // cursor position in tracker area
    unsigned int current_pattern = 0;
    unsigned int current_instrument = 0;
    unsigned int current_octave = 4;
    int cursor_x = 0;
    int cursor_row = 0;
    // playing pos. in order, or -1 if playing single pattern
    int playing_order = 0;

    // playback state returned by synth, written from audio callback
    // so protected by mutex
    std::mutex playstate_mutex;
    unsigned int playing_pattern = 0;
    unsigned int playing_row = 0;

    // Track mute flags
    std::vector<bool>mute_flags = std::vector<bool>(TrackerData::max_num_tracks, false);

    Button order_shrink;
    Button order_grow;
    int order_cursor_pos = 0;

    TextBox tempo_textbox;
    TextBox pattern_rows_textbox;
    TextBox pattern_tracks_textbox;
    TextBox master_hb_coeff_textbox;
    TextBox master_hb_mix_textbox;

    const unsigned int octave_min = 0;
    const unsigned int octave_max = 8;

    void cursor_move(int x, int y);
    void cursor_move_track(int dir);

    bool edit(int key_char);
    void del_row();
    void transpose_track(int semitones);
    void change_octave(int dir);
    void change_instrument(int dir);
    void change_pattern(int dir);

    void order_cursor_move(int dir);
    void current_order_adjust(int dir);
    void current_order_set(int pattern);

    // adjust size length of order list
    void order_adjust(int dir);

    void order_insert();
    void order_del();

    // return positions of order list numbers
    int order_x(int order_idx);
    int order_y(int order_idx);

    void set_tempo_cb(const std::string &value);
    void set_pattern_rows_cb(const std::string &value);
    void set_pattern_tracks_cb(const std::string &value);
    void set_master_hb_coeff_cb(const std::string &value);
    void set_master_hb_mix_cb(const std::string &value);

public:
    TrackerWindow(int x, int y, int width, int height, SDL_Renderer *renderer,
		  TTF_Font *_text_font, ModalPromptInterface &prompt_input,
		  TrackerData &data);

    void key_down(const SDL_Keysym &sym) override;
    void key_up(const SDL_Keysym &sym) override;

    void update() override;

    // get no. of currently shown pattern
    size_t get_current_pattern();
    int get_current_row();
    int get_order_cursor_pos();

    void report_playstate(unsigned int current_pattern, unsigned int current_row);

    void update_data(TrackerData &data) override;

    Json::Value as_json() override;
    void from_json(Json::Value &json) override;
};
