#include "editor_window.hpp"
#include "fonts.hpp"
#include <string>
#include <sstream>
#include <map>
#include <iostream>
#include <functional>
#include <GL/gl.h>
#include <SDL_opengl.h>

EditorWindow::Button::Button(const std::string &label_,
			     std::function<void(void)>callback_,
			     int x_, int y_,
			     int width_, int height_)
    : label(label_), callback(callback_), x(x_), y(y_) {
    if (width_ >= 0) {
	width = width_;
    } else {
	width = default_width;
    }
    if (height_ >= 0) {
	height = height_;
    } else {
	height = default_height;
    }
}

bool EditorWindow::Button::is_inside(int px, int py) {
    return (px >= x && py >= y &&
	    px < (x + width) && py < (y + height));
}

EditorWindow::TextBox::TextBox(int x_, int y_, size_t max_size_, const std::string &initial)
    : x(x_), y(y_),
      height(UI::Text::char_height * 2), width(max_size_ * 2 * UI::Text::char_width),
      max_size(max_size_), value(initial) {
}

EditorWindow::TextBox::TextBox(int x_, int y_, size_t max_size_, const std::string &initial,
			       std::function<void(std::string)> callback)
    : TextBox(x_, y_, max_size_, initial) {
    optional_callback = callback;
}

bool EditorWindow::TextBox::is_inside(int px, int py) {
    return (px >= x && py >= y &&
	    px < (x + width) && py < (y + height));
}

EditorWindow::ModalInput::ModalInput(int x_, int y_,
				     std::string label_,
				     std::string value_)
    : x(x_), y(y_), label(label_), value(value_), active(false) {
}

void EditorWindow::register_(Button &button) {
    reg_buttons.push_back(button);
}

void EditorWindow::register_(TextBox &textbox) {
    reg_textboxes.push_back(textbox);
}

void EditorWindow::box_assign(TextBox &box, std::string val) {
    box.value = val;
    if (box.optional_callback.has_value()) {
	box.optional_callback.value()(val);
    }
}

void EditorWindow::str_assign(std::string &dst, std::string val) {
    dst = val;
}

bool EditorWindow::in_child(const EditorWindow *child, int x, int y) {
    return (x >= child->offset_x && y >= child->offset_y &&
	    x <= (child->offset_x + child->width) &&
	    y <= (child->offset_y + child->height));
}

EditorWindow::EditorWindow(int x, int y, int _width, int _height, SDL_Renderer *_renderer,
			   TTF_Font *_text_font, ModalPromptInterface &prompt_input_)
    : renderer(_renderer), text_font(_text_font), prompt_input(prompt_input_),
      offset_x(x), offset_y(y), width(_width), height(_height) {
}

EditorWindow::~EditorWindow() {
}

void EditorWindow::focus_enter() {
    in_focus = true;
}

void EditorWindow::focus_leave() {
    in_focus = false;
}

void EditorWindow::draw_rect(int x, int y, int width, int height, const Color &color,
			     bool filled) {
    // std::cerr << "draw_rect(" << x << ", " << y << ", "
    //	      << width << ", " << height << ")" << std::endl;
    glPushMatrix();
    set_color(color);
    glTranslated(offset_x + x, offset_y + y, 0.0);
    if (filled) {
	glBegin(GL_TRIANGLE_STRIP);
	glVertex3d(0.0, height, 0.0);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(width, height, 0.0);
	glVertex3d(width, 0.0, 0.0);
    } else {
	glBegin(GL_LINE_STRIP);
	glVertex3d(0.0, 0.0, 0.0);
	glVertex3d(width, 0.0, 0.0);
	glVertex3d(width, height, 0.0);
	glVertex3d(0.0, height, 0.0);
	glVertex3d(0.0, 0.0, 0.0);
    }
    glEnd();
    glPopMatrix();
}

void EditorWindow::draw_rect(const Rect &rect, const Color &color, bool filled) {
    draw_rect(rect.x, rect.y, rect.w, rect.h, color, filled);
}

void EditorWindow::draw_line(int x1, int y1, int x2, int y2, const Color &color) {
    glPushMatrix();
    set_color(color);
    glTranslated(offset_x + x1, offset_y + y1, 0.0);
    glBegin(GL_LINES);
    glVertex3d(0.0, 0.0, 0.0);
    glVertex3d(x2 - x1, y2 - y1, 0.0);
    glEnd();
    glPopMatrix();
}

void EditorWindow::draw_text_impl(int x, int y, const std::string &text,
				  const Color &color_fg, const Color &color_bg,
				  int width_mult, int height_mult) {
    // std::cerr << "draw_text(" << x << ", " << y << ", \""
    //	      << text << "\")" << std::endl;
    // std::cerr << "offsets " << offset_x << " " << offset_y << std::endl;

    SDL_Color text_color = {255, 255, 255, 255};
    text_color.r = color_fg.r;
    text_color.g = color_fg.g;
    text_color.b = color_fg.b;
    text_color.a = color_fg.a;

    SDL_Surface *text_surf = TTF_RenderText_Solid(text_font, text.c_str(), text_color);
    // std::cerr << "surf size: " << text_surf->w << ", " << text_surf->h << std::endl;

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, text_surf);

    glPolygonMode(GL_FRONT, GL_FILL);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    float t_width, t_height;
    SDL_GL_BindTexture(texture, &t_width, &t_height);
    // std::cerr << "bound texture size: " << t_width << ", " << t_height << std::endl;

    glPushMatrix();
    set_color(color_bg);
    glTranslated(offset_x + x, offset_y + y, 0.0);
    glBegin(GL_QUADS);
    glTexCoord2d(0, 0);
    glVertex2d(0, 0);
    glTexCoord2d(t_width, 0);
    glVertex2d(UI::Text::char_width * width_mult * text.size(), 0);
    glTexCoord2d(t_width, t_height);
    glVertex2d(UI::Text::char_width * width_mult * text.size(),
	       UI::Text::char_height * height_mult);
    glTexCoord2d(0, t_height);
    glVertex2d(0, UI::Text::char_height * height_mult);
    glEnd();
    glPopMatrix();

    SDL_GL_UnbindTexture(texture);
    SDL_DestroyTexture(texture);
    SDL_FreeSurface(text_surf);
}

bool EditorWindow::just_changed() {
    for (auto &child : children) {
	if (child->just_changed()) {
	    changed = true;
	}
    }
    bool tmp = changed;
    changed = false;
    return tmp;
}

void EditorWindow::draw_text(int x, int y, const std::string &text,
			     const Color &color_fg, const Color &color_bg) {
    draw_text_impl(x, y, text, color_fg, color_bg, 4, 2);
}

void EditorWindow::draw_text_hw(int x, int y, const std::string &text,
			     const Color &color_fg, const Color &color_bg) {
    draw_text_impl(x, y, text, color_fg, color_bg, 2, 2);
}

void EditorWindow::draw_text_small(int x, int y, const std::string &text,
			     const Color &color_fg, const Color &color_bg) {
    draw_text_impl(x, y, text, color_fg, color_bg, 1, 1);
}

void EditorWindow::draw_modal_input(int x, int y,
				    const std::string &label, const std::string &value) {
    int edge_pad = 2;
    int h_total = 2 * (UI::Text::char_height * 2) + UI::Text::row_gap + edge_pad * 2;
    int w_total = (2 * std::max(label.size(), value.size() + 1) * (UI::Text::char_width * 2)
		   + edge_pad * 2);
    draw_rect(x, y, w_total, h_total, UI::Colors::default_color_bg, true);
    draw_rect(x, y, w_total, h_total);
    draw_text(x + edge_pad, y + edge_pad, label);
    draw_text(x + edge_pad, y + edge_pad + UI::Text::char_height * 2 + UI::Text::row_gap,
	      value + "_");
}

void EditorWindow::mouse_move(int x, int y, [[maybe_unused]] bool inside) {
    for (auto &child : children) {
	child->mouse_move(x - child->offset_x, y - child->offset_y,
			  in_child(child, x, y));
    }
}

void EditorWindow::mouse_click(int button, int x, int y, [[maybe_unused]] bool inside) {
    for (auto &child : children) {
	child->mouse_click(button, x - child->offset_x, y - child->offset_y,
			   in_child(child, x, y));
    }
    switch (button) {
    case SDL_BUTTON_LEFT:
	for (auto &button_r : reg_buttons) {
	    auto &button = button_r.get();
	    if (button.is_inside(x, y)) {
		button.callback();
	    }
	}
	break;
    case SDL_BUTTON_MIDDLE:
	for (auto &textbox_r : reg_textboxes) {
	    auto &textbox = textbox_r.get();
	    if (textbox.is_inside(x, y)) {
		prompt_input.prompt_input("Value",
					  textbox.value,
					  std::bind(&EditorWindow::box_assign, std::ref(textbox),
						    std::placeholders::_1));
	    }
	}
	break;
    }
}

void EditorWindow::mouse_release(int button, int x, int y, [[maybe_unused]] bool inside) {
    for (auto &child : children) {
	child->mouse_release(button, x - child->offset_x, y - child->offset_y,
			     in_child(child, x, y));
    }
}

void EditorWindow::key_down(const SDL_Keysym &sym) {
    for (auto &child : children) {
	child->key_down(sym);
    }
}

void EditorWindow::key_up(const SDL_Keysym &sym) {
    for (auto &child : children) {
	child->key_up(sym);
    }
}

void EditorWindow::edit_text(const std::string &input, bool finished) {
    for (auto &child : children) {
	child->edit_text(input, finished);
    }
}

void EditorWindow::set_color(const Color &color) {
    glColor4b(color.r >> 1, color.g >> 1, color.b >> 1, color.a >> 1);
}

void EditorWindow::draw_button(const EditorWindow::Button &button) {
    draw_text_hw(button.x + 1, button.y + 1, button.label);
    draw_rect(button.x, button.y, button.width, button.height);
}

void EditorWindow::draw_textbox(const EditorWindow::TextBox &textbox) {
    int x = textbox.x + 1;
    if (textbox.label.size() > 0) {
	draw_text_hw(x, textbox.y + 1, textbox.label);
	x += UI::Text::char_width * 2 * textbox.label.size();
    }
    draw_text_hw(x, textbox.y + 1, textbox.value);
    draw_rect(textbox.x, textbox.y, textbox.width, textbox.height);
}

void EditorWindow::update() {
    for (auto &button : reg_buttons) {
	draw_button(button);
    }
    for (auto &textbox : reg_textboxes) {
	draw_textbox(textbox);
    }
}
