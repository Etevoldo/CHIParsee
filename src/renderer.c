#include <stdlib.h>
#include <stdio.h>
#include "renderer.h"
#include "instructions.h"
#include "raylib.h"
#include "types.h"
#include "raygui.h"
#include "stack.h"

#define ON_COLOR CLITERAL(Color){ 155, 188, 15, 255 }
#define OFF_COLOR CLITERAL(Color){ 15, 56, 15, 255 }
#define RECT(x, y, xwidth, y_width) ((Rectangle) { x, y, xwidth, y_width })
#define INST_DEBUG_STRING_SIZE 11 // "XXXX = XXXX"

static const int inst_to_display;
static int inst_active;
static int inst_focus;
static int inst_scrollIndex;
static char **inst_list;
static bool is_paused;
static bool is_step;
static float IPF;
static char bp_text[64];
static bool bp_is_edit_on;
static float audio_freq_slider;
static float audio_volume;
static Quirks quirks;
static List bp_list;

void draw(IO *io, Regs *regs)
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
        draw_debug(regs, io->keys_down);
        #endif

        draw_gui(regs);
    EndDrawing();
}

void init_gui()
{
    inst_active = RAM_SIZE / 4;
    inst_focus = -1;
    inst_scrollIndex = RAM_SIZE / 4 - 10;
    inst_list = NULL;
    is_paused = false;
    is_step = false;
    IPF = 15.0f;
    bp_is_edit_on = false;
    audio_freq_slider = 440.0f;
    audio_volume = 7.0f;
    bp_list = list_create(64);
    inst_list = alloc_debugger_inst_array();
    quirks = init_quirks();
}

void draw_gui(Regs *regs)
{
    const int gui_y = DISPLAY_HEIGHT * SCALE + BORDER_WIDTH;
    const int gui_x = BORDER_WIDTH;

    // volume slider
    GuiSlider(
        RECT(gui_x + 20, gui_y, 200, 20),
        "Vol",
        TextFormat("%2.f%%", audio_volume * 100),
        &audio_volume,
        0.0f,
        1.0f);

    // volume Frequency slider
    // converting to frequency cause raygui sliders must be float
    GuiSlider(
        RECT(gui_x + 20, gui_y + 40, 200, 20),
        "Freq",
        TextFormat("%dHz", audio_freq_slider),
        &audio_freq_slider,
        50.0f,
        2000.0f);

    // IPF
    GuiSlider(
        RECT(gui_x + 20, gui_y + 80, 200, 20),
        "IPF",
        TextFormat("%dIPF", (int)IPF),
        &IPF,
        1.0f,
        25.0f);

    const int second_row_x = gui_x + 270;
    // Quirks
    GuiCheckBox(
        RECT(second_row_x, gui_y, 20, 20),
        "display wait",
        &(quirks.is_display_wait));
    GuiCheckBox(
        RECT(second_row_x, gui_y + 40, 20, 20),
        "8xy6 & 8xye: Vy into Vx before shift",
        &quirks.is_8xy6e_vy_into_vx);
    GuiCheckBox(
        RECT(second_row_x, gui_y + 80, 20, 20),
        "bxnn instead of bnnn",
        &quirks.is_bnnn_vx);
    GuiCheckBox(
        RECT(second_row_x, gui_y + 120, 20, 20),
        "fx55 & fx65: update I",
        &quirks.is_fx565_change_I);
}

char ** alloc_debugger_inst_array()
{
    char **inst_list;
    inst_list = (char **) malloc(INST_TO_DISPLAY * sizeof(char **));
    if (inst_list == NULL) {
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < INST_TO_DISPLAY; i++) {
        (inst_list)[i] = (char *) malloc(INST_DEBUG_STRING_SIZE + 1);
        if (inst_list[i] == NULL) {
            exit(EXIT_FAILURE);
        }
    }
    return inst_list;
}

void load_debugger_inst_array(u8 *ram)
{
    for (int j = 0, i = 0; i < INST_TO_DISPLAY; j += 2, i++) {
        sprintf(
            inst_list[i],
            "%.4X: %.2X%.2X\n",
            j, ram[j], ram[j + 1]);
    }
}

// free debug instructions string list
void free_debugger_inst_array()
{
    for (int i = 0; i < INST_TO_DISPLAY; i++) {
        free(inst_list[i]);
    }
    free(inst_list);
}

void draw_debug(Regs *regs, bool keys_down[])
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
    if (!is_paused) {
        update_scroll(regs->pc);
    }
    GuiGroupBox(
        RECT(debug_x + 138, debug_y, 200, 620),
        "Instructions");

    GuiListViewEx(
        RECT(debug_x + 148, debug_y + 10, 180, 600),
        inst_list,
        INST_TO_DISPLAY,
        &inst_scrollIndex,
        &inst_active,
        &inst_focus
    );

    // Pause/unpause and step
    const int debug_buttons_y = debug_y + 285;
    GuiIconName icon = is_paused ? ICON_PLAYER_PLAY : ICON_PLAYER_PAUSE;
    Rectangle pause_icon = RECT(debug_x, debug_buttons_y, 30, 30);
    if (GuiButton(pause_icon, GuiIconText(icon, ""))) {
        is_paused = !is_paused;
    }
    Rectangle step_icon = RECT( debug_x + 35, debug_buttons_y, 30, 30);
    if (GuiButton(step_icon, GuiIconText(ICON_STEP_OVER, ""))) {
        is_step = true;
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
            RECT(bp_x, padding, 100, 30), bp_text,
            20, bp_is_edit_on)) {
            bp_is_edit_on = !bp_is_edit_on;
       }

    GuiIconName bp_icon = ICON_CIRCLE_ADD_FILL;
    Rectangle bp_rec = RECT(bp_x + 110, padding, 30, 30);
    if (GuiButton(bp_rec, GuiIconText(bp_icon, ""))) {
        char *end;
        const int address = strtol(bp_text, &end, 16);
        if (address) {
            list_push(&bp_list, address);
        }
        // debug
        list_print(&bp_list);
    }

    GuiLabel(
        RECT(bp_x, 30 + padding, 100, 20),
        "Breakpoints:"
    );

    int bp_length = bp_list.length;
    for (int i = 0; i < bp_length; i++) {
        GuiLabel(
            RECT(bp_x, 50 + padding + 30 * i, 50, 30),
            TextFormat("%.4X", bp_list.arr[i])
        );
        if (GuiButton(
            RECT(bp_x + 55, 50 + padding + 30 * i, 30, 30),
            GuiIconText(ICON_CROSS, ""))) {
            list_remove(&bp_list, i);
        }
    }
}

// update global variables of for the instructions scroll list
void update_scroll(u16 pc)
{
    inst_scrollIndex = pc / 2 - 10;
    inst_active = pc / 2;
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
