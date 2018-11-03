#pragma once

#include "types.hpp"
#include "editor_window.hpp"
#include "tracker_data.hpp"
#include <map>
#include <set>

class PatchEditorWindow : public EditorWindow {
  protected:

    static constexpr int MODULE_SIZE = 64;
    static constexpr int MODULE_AREA_PADDING = 8;
    static constexpr int NUM_INSTRUMENTS = 8;

    class EditorModule {
    protected:
	virtual int hovered_input_impl(int x, int y);
    public:
	virtual ~EditorModule();

	std::string label;
	// String value for each module param, input by user. We preserve this as-is
	// and convert to actual module value after modification (or module load).
	// This is because we don't want any transformations of string representation
	// of value when it can't be represented by float exactly
	// (i.e. prevent any 0.99999 -> 0.9999900000002 style issues).
	std::vector<std::string> string_values;

	int x = 64;
	int y = 64;
	int width = PatchEditorWindow::MODULE_SIZE;
	int height = PatchEditorWindow::MODULE_SIZE;

	// Visible only on this page (except master is visible on all)
	int page = 0;

	// Master out is special in many ways: indestructible, unmovable,
	// only one input, excluded from syth data dump, etc.
	bool is_master_out = false;

	// If >= 0, module is triggered by that instrument
	std::string string_trigger;
	int trigger_instrument = -1;

	// All tracks that actually need to have this module enabled. Replaced with
	// a no-op on others. TODO: make this optional so that we can still generate
	// more compact (slower) songs for 4k:s
	std::set<int> using_tracks;

	Module module;

	virtual void draw(int x, int y);
	virtual bool is_inside(int x, int y);
	virtual int hovered_param(int x, int y);
	virtual int hovered_input(int x, int y);

	void adjust_size();
    };

    // Labels for different module types
    std::map<Module::ModuleType, std::string> module_disp_types = {
	{Module::TYPE_OSC, "osc"},
	{Module::TYPE_FILTER, "flt"},
	{Module::TYPE_ENV, "env"},
	{Module::TYPE_COMP, "cmp"},
	{Module::TYPE_REVERB, "rev"},
	{Module::TYPE_CHORUS, "cho"}
    };

    // Labels for displayed param values for each module type
    std::map<Module::ModuleType, std::vector<std::string>> module_param_labels = {
	{Module::TYPE_OSC, {"gain", "fmod", "add ", "dtne"}},
	{Module::TYPE_FILTER, {"in  ", "coff", "fdbk", "   "}},
	{Module::TYPE_ENV, {"att ", "swtc", "dcay", "stg "}},
	{Module::TYPE_COMP, {"in  ", "thre", "att ", "rel "}},
	{Module::TYPE_REVERB, {"in  ", "taps", "fdbk", "lp  "}},
	{Module::TYPE_CHORUS, {"in", "time", "madd", "mamp"}}
    };

    static constexpr int MASTER_OUT_IDX = 0;
    std::vector<EditorModule> modules;

    std::vector<TextBox> instr_textboxes;
    std::vector<Button> action_buttons;

    // Primary and secondary instruments for each track
    std::vector<std::pair<int, int>> track_instruments;

    // Trigger points for each itstrument. Each instrument triggers
    // one oscillator and one envelope (or N modules in 64k mode: TODO, make option for this).
    std::vector<std::pair<int, int>> instrument_triggers;

    // Index of hovered module, or -1 if none hovered
    int hovered_module = -1;
    // Index of hovered module float parameter, or -1 if none hovered
    int hovered_param = -1;
    // INdex of hovered non-float-param input, or -1 if none hovered
    int hovered_input = -1;
    // Index of selected module (left click), or -1 if none selected
    int selected_module = -1;

    bool dragging = false;
    int drag_offset_x = 0;
    int drag_offset_y = 0;

    // Modules are organized in multiple pages to make more fit
    static constexpr int num_pages = 12;
    int shown_page = 0;
    Button prev_page;
    Button next_page;

    const Color hovered_color = {0, 127, 127, 127};
    const Color selected_color = {127, 63, 0, 127};
    const Color hovered_cpoint_color = {127, 100, 100, 127};

    // Clamp module position to editor area
    void clamp_module_pos(EditorModule &module);
    // Return true if connection graph contains a cycle (and thus can't be
    // converted to synth configuration)
    bool has_connection_cycle();
    // Sync synth module parameter values from string values in UI
    void update_params_from_ui();
    //
    void update_track_instruments_from_ui();

    void change_page(int dir);

  public:
    PatchEditorWindow(int x, int y, int width, int height, int num_tracks,
		      SDL_Renderer *renderer, TTF_Font *font,
		      ModalPromptInterface &prompt_input);
    ~PatchEditorWindow();

    void from_json(Json::Value &json);
    Json::Value as_json();

    void mouse_move(int x, int y, bool inside) override;
    void mouse_click(int button_idx, int x, int y, bool inside) override;
    void mouse_release(int button_idx, int x, int y, bool inside) override;

    void key_down(const SDL_Keysym &sym) override;
    void key_up(const SDL_Keysym &sym) override;

    void draw_module(const EditorModule &module, const Color &edge_color, bool is_hovered,
		      int idx);
    void update() override;

    // Button actions
    void new_module(Module::ModuleType type);
    void del_module();

    // Convert patch editor state to synth instrument & trigger
    // point config. Other parts of &data are not modified.
    void update_data(TrackerData &data) override;
};
