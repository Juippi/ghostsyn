#pragma once

#include "types.hpp"
#include "ui_constants.hpp"
#include <SDL.h>
#include <SDL_ttf.h>
#include <memory>
#include <functional>
#include <vector>
#include <optional>


class ModalPromptInterface {
public:
    virtual void prompt_input(const std::string &label,
			      const std::string &initial,
			      std::function<void(std::string)> callback) = 0;
    virtual void cancel_prompt() = 0;
};

/**
 Editor window component, which may be a sub"window" of another EditorWindow.
 Delivers incoming events & draw requests to children.

 Coordinates for draw methods are relative to the top left of the parent
 window.

 TODO: can we create some kind of mask to prevent us from drawing outside our area?
*/

class EditorWindow  {
private:
    bool in_child(const EditorWindow *child, int x, int y);

protected:
    bool in_focus = false;
    bool changed = false; // if true, needs redraw soon

    SDL_Renderer *renderer = 0;
    TTF_Font *text_font = 0;
    ModalPromptInterface &prompt_input;

    static const Color default_color_fg;
    static const Color default_color_bg;

    std::vector<EditorWindow *> children;
    
    void draw_text_impl(int x, int y, const std::string &text, const Color &color_fg,
			const Color &color_bg, int width_mult, int height_mult);

    class Button {
    public:
	Button(const std::string &label,
	       std::function<void(void)> callback,
	       int x, int y,
	       int width = -1, int height = -1);
	std::string label;
	std::function<void(void)> callback;
	int x;
	int y;
	int height;
	int width;

	static constexpr int default_height = 34;
	static constexpr int default_width = 64;

	bool is_inside(int x, int y);
    };

    class TextBox {
    public:
	TextBox(int x, int y, size_t max_size, const std::string &initial = "");
	TextBox(int x, int y, size_t max_size, const std::string &initial,
		std::function<void(std::string)> callback);

	std::optional<std::function<void(std::string)>> optional_callback;
	int x;
	int y;
	int height;
	int width;
	size_t max_size;
	std::string label;
	std::string value;

	bool is_inside(int x, int y);
    };

    class ModalInput {
    public:
	ModalInput(int x, int y,
		   std::string label = "",
		   std::string value = "");
	int x;
	int y;
	std::string label;
	std::string value;
	bool active;
    };

private:
    std::vector<std::reference_wrapper<Button>> reg_buttons;
    std::vector<std::reference_wrapper<TextBox>> reg_textboxes;

protected:
    void register_(Button &);
    void register_(TextBox &);

    static void str_assign(std::string &str, std::string val);
    static void box_assign(TextBox &box, std::string val);

public:
    const int offset_x = 0;
    const int offset_y = 0;
    const int width = 0;
    const int height = 0;

    EditorWindow(int x, int y, int width, int height, SDL_Renderer *renderer,
		 TTF_Font *_text_font, ModalPromptInterface &prompt_input);
    virtual ~EditorWindow();

    virtual void mouse_move(int x, int y, bool inside);
    virtual void mouse_click(int button_idx, int x, int y, bool inside);
    virtual void mouse_release(int button_idx, int x, int y, bool inside);

    virtual void key_down(const SDL_Keysym &sym);
    virtual void key_up(const SDL_Keysym &sym);

    virtual void edit_text(const std::string &input, bool finished);

    virtual void focus_enter();
    virtual void focus_leave();
    
    void draw_rect(int x, int y, int width, int height, const Color &color = default_color_fg,
		   bool filled = false);
    void draw_rect(const Rect &rect, const Color &color = default_color_fg,
		   bool filled = false);
    void draw_line(int x1, int y1, int x2, int y2, const Color &color = default_color_fg);
    void draw_text(int x, int y, const std::string &text,
		   const Color &color_fg = default_color_fg,
		   const Color &color_bg = default_color_bg);
    void draw_text_hw(int x, int y, const std::string &text,
		      const Color &color_fg = default_color_fg,
		      const Color &color_bg = default_color_bg);
    void draw_text_small(int x, int y, const std::string &text,
		      const Color &color_fg = default_color_fg,
		      const Color &color_bg = default_color_bg);

    void draw_modal_input(int x, int y, const std::string &label, const std::string &value);

    void set_color(const Color &color);

    bool just_changed();

    void draw_button(const Button &tbutton);
    void draw_textbox(const TextBox &textbox);

    // Draw contents of window. This is usually called after just_changed()
    // returns true, or the window becomes unoccluded or visible.
    virtual void update();
};
