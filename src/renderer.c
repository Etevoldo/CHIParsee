#include <stdlib.h>
#include <stdio.h>
#include "renderer.h"
#include "raylib.h"
#include "types.h"
#include "raygui.h"
#include "stack.h"

#define ON_COLOR CLITERAL(Color){ 155, 188, 15, 255 }
#define OFF_COLOR CLITERAL(Color){ 15, 56, 15, 255 }
#define RECT(x, y, xwidth, y_width) ((Rectangle) { x, y, xwidth, y_width })
#define INST_DEBUG_STRING_SIZE 11 // "XXXX = XXXX"

void draw(IO *io, Regs *regs, GuiVars *gui_vars)
{
    io->display_wait = false;
    io->last_key_pressed = NO_KEY;

    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            int index = x + y * DISPLAY_WIDTH;
            if (!io->display[index]) {
                ImageDrawPixel(&io->screen_image, x , y, OFF_COLOR);
            }
            else ImageDrawPixel(&io->screen_image, x , y, ON_COLOR);
        }
    }
    UpdateTexture(io->screen_texture, io->screen_image.data);

    BeginDrawing();
        ClearBackground(BLACK);

        DrawTextureEx(
            io->screen_texture,
            (Vector2) { 0, 0 },
            0.0f,
            (float) SCALE,
            WHITE);

        #ifdef DEBUG_ON
        draw_debug(regs, gui_vars, io->keys_down);
        #endif

        draw_gui(regs, gui_vars);
    EndDrawing();
}

GuiVars init_gui_vars()
{
    GuiVars gui_vars = (GuiVars) {
        .inst_to_display = RAM_SIZE / 2,
        .inst_active = RAM_SIZE / 4,
        .inst_focus = -1,
        .inst_scrollIndex = RAM_SIZE / 4 - 10,
        .inst_list = NULL,
        .is_paused = false,
        .is_step = false,
        .IPF = 15,
        .bp_is_edit_on = false
    };
    gui_vars.bp_list = list_create(64);

    return gui_vars;
}

void draw_gui(Regs *regs, GuiVars *gui_vars)
{
    const int gui_y = DISPLAY_HEIGHT * SCALE + BORDER_WIDTH;
    const int gui_x = BORDER_WIDTH;

    // volume slider
    GuiSlider(
        RECT(gui_x + 20, gui_y, 200, 20),
        "Vol",
        TextFormat("%2.f%%", gui_vars->audio_data.volume * 100),
        &(gui_vars->audio_data.volume),
        0.0f,
        1.0f);

    // volume Frequency slider
    // converting to frequency cause raygui sliders must be float
    float freq_slider = (float) gui_vars->audio_data.frequency;
    GuiSlider(
        RECT(gui_x + 20, gui_y + 40, 200, 20),
        "Freq",
        TextFormat("%dHz", gui_vars->audio_data.frequency),
        &freq_slider,
        50.0f,
        2000.0f);
    gui_vars->audio_data.frequency = freq_slider;

    // IPF
    float IPF = (float) gui_vars->IPF;
    GuiSlider(
        RECT(gui_x + 20, gui_y + 80, 200, 20),
        "IPF",
        TextFormat("%dIPF", (int)IPF),
        &IPF,
        1.0f,
        25.0f);
    gui_vars->IPF = (int) IPF;

    const int second_row_x = gui_x + 270;
    // Quirks
    GuiCheckBox(
        RECT(second_row_x, gui_y, 20, 20),
        "display wait",
        &(gui_vars->quirks.is_display_wait));
    GuiCheckBox(
        RECT(second_row_x, gui_y + 40, 20, 20),
        "8xy6 & 8xye: Vy into Vx before shift",
        &(gui_vars->quirks.is_8xy6e_vy_into_vx));
    GuiCheckBox(
        RECT(second_row_x, gui_y + 80, 20, 20),
        "bxnn instead of bnnn",
        &(gui_vars->quirks.is_bnnn_vx));
    GuiCheckBox(
        RECT(second_row_x, gui_y + 120, 20, 20),
        "fx55 & fx65: update I",
        &(gui_vars->quirks.is_fx565_change_I));
}

void alloc_debugger_inst_array(char ***inst_list)
{
    *inst_list = (char **) malloc(INST_TO_DISPLAY * sizeof(char **));
    if (*inst_list == NULL) {
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < INST_TO_DISPLAY; i++) {
        (*inst_list)[i] = (char *) malloc(INST_DEBUG_STRING_SIZE + 1);
        if ((*inst_list)[i] == NULL) {
            exit(EXIT_FAILURE);
        }
    }
}

void load_debugger_inst_array(u8 *ram, char **inst_list)
{
    for (int j = 0, i = 0; i < INST_TO_DISPLAY; j += 2, i++) {
        sprintf(
            inst_list[i],
            "%.4X: %.2X%.2X\n",
            j, ram[j], ram[j + 1]);
    }
}

// free debug instructions string list
void free_debugger_inst_array(char **inst_list)
{
    for (int i = 0; i < INST_TO_DISPLAY; i++) {
        free(inst_list[i]);
    }
    free(inst_list);
}

void draw_debug(Regs *regs, GuiVars *gui_vars, bool keys_down[])
{

    const char *text;

    const int debug_x = DISPLAY_WIDTH * SCALE + 10;
    const int debug_y = 10;
    const int label_width = 60;
    const int label_height = 50;
    const int label_spacing = 15;
    const int padding = 10;

    // keys state
    GuiGroupBox((Rectangle) {debug_x, debug_y, 70, 280 }, "Keys");
    for (int i = 0; i < N_OF_KEYS; i++) {
        text = TextFormat(
            "%0.1X = %s", i, keys_down[i] ? "DOWN" : "UP");
        GuiLabel(
            (Rectangle) {
                debug_x + padding,
                label_spacing * i,
                label_width,
                label_height },
            text);
    }

    // Registers V 0 to F
    GuiGroupBox(RECT(debug_x + 69, debug_y, 70, 280), "V.Regs");
    for (int i = 0; i < 16; i++) {
        text = TextFormat("V%0.1X = %0.2X", i, regs->v[i]);
        GuiLabel(
            RECT(debug_x + 79, label_spacing * i,
                 label_width, label_height),
            text);
    }
    // Instructions
    if (!gui_vars-> is_paused) {
        update_scroll(regs, gui_vars);
    }
    GuiGroupBox(
        RECT(debug_x + 138, debug_y, 200, 620),
        "Instructions");

    GuiListViewEx(
        RECT(debug_x + 148, debug_y + 10, 180, 600),
        gui_vars->inst_list,
        gui_vars->inst_to_display,
        &(gui_vars->inst_scrollIndex),
        &(gui_vars->inst_active),
        &(gui_vars->inst_focus)
    );

    // Pause/unpause and step
    const int debug_buttons_y = debug_y + 285;
    GuiIconName icon = gui_vars->is_paused ? ICON_PLAYER_PLAY : ICON_PLAYER_PAUSE;
    Rectangle pause_icon = RECT(debug_x, debug_buttons_y, 30, 30);
    if (GuiButton(pause_icon, GuiIconText(icon, ""))) {
        gui_vars->is_paused = !gui_vars->is_paused;
    }
    Rectangle step_icon = RECT( debug_x + 35, debug_buttons_y, 30, 30);
    if (GuiButton(step_icon, GuiIconText(ICON_STEP_OVER, ""))) {
        gui_vars->is_step = true;
    }

    // stack
    u16 address;
    GuiGroupBox(RECT(debug_x, debug_y + 340, 70, 280 ), "Stack");
    for (int i = 0; i < 16; i++) {
        address = regs->stack.arr[i];
        text = TextFormat("%0.4X", address);
        GuiLabel(
            RECT(debug_x + padding, debug_y + 340 + label_spacing * i,
                label_width, label_height),
            text);
    }

    // other Registers
    GuiGroupBox(
        RECT(debug_x + 69, debug_y + 340, 70, 280),
        "Other\nRegs.");
    // I registers
    GuiLabel(
        RECT(debug_x + 69 + padding, debug_y + 340 + padding,
            label_width, label_height ),
        TextFormat("I  = %0.4X", regs->index));
    // PC
    GuiLabel(
        RECT(debug_x + 69 + padding, debug_y + 340 + padding + label_spacing,
            label_width, label_height ),
        TextFormat("PC = %0.4X", regs->pc));
    // Sound
    GuiLabel(
        RECT(debug_x + 69 + padding, debug_y + 340 + padding + label_spacing*2,
            label_width, label_height ),
        TextFormat("ST = %0.4X", regs->sound));
    // Delay
    GuiLabel(
        RECT(debug_x + 69 + padding, debug_y + 340 + padding + label_spacing*3,
            label_width, label_height ),
        TextFormat("DT = %0.4X", regs->delay));

    // Breakpoints
    const int bp_x = debug_x + 138 + 200 + padding;
    if (GuiTextBox(
            RECT(bp_x, padding, 100, 30), gui_vars->bp_text,
            20, gui_vars->bp_is_edit_on)) {
            gui_vars->bp_is_edit_on = !gui_vars->bp_is_edit_on;
       }

    GuiIconName bp_icon = ICON_CIRCLE_ADD_FILL;
    Rectangle bp_rec = RECT(bp_x + 110, padding, 30, 30);
    if (GuiButton(bp_rec, GuiIconText(bp_icon, ""))) {
        char *end;
        const int address = strtol(gui_vars->bp_text, &end, 16);
        if (address) {
            list_push(&gui_vars->bp_list, address);
        }
        // debug
        list_print(&gui_vars->bp_list);
    }

    GuiLabel(
        RECT(bp_x, 30 + padding, 100, 20),
        "Breakpoints:"
    );

    int bp_length = gui_vars->bp_list.length;
    for (int i = 0; i < bp_length; i++) {
        GuiLabel(
            RECT(bp_x, 50 + padding + 30 * i, 50, 30),
            TextFormat("%.4X", gui_vars->bp_list.arr[i])
        );
        if (GuiButton(
            RECT(bp_x + 55, 50 + padding + 30 * i, 30, 30),
            GuiIconText(ICON_CROSS, ""))) {
            list_remove(&gui_vars->bp_list, i);
        }
    }
}

// update global variables of for the instructions scroll list
void update_scroll(Regs *regs, GuiVars *gui_vars)
{
    gui_vars->inst_scrollIndex = regs->pc / 2 - 10;
    gui_vars->inst_active = regs->pc / 2;
}

void clear_display(bool display[])
{
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            int index = x + y * DISPLAY_WIDTH;
            display[index] = false;
        }
    }
}

bool drawPixel(int x, int y, bool isBitOn, bool display[])
{
    int index = x + y * DISPLAY_WIDTH;
    bool collision = false;

    if (isBitOn) {
        if (display[index]) {
            display[index] = false;
            collision = true;
        }
        else if (!display[index]) {
            display[index] = true;
        }
    }

    return collision;
}
