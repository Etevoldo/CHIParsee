#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "renderer.h"
#include "instructions.h"
#include "raylib.h"
#include "types.h"

#include "raygui.h"
#include "gui_window_file_dialog.h"

#define ON_COLOR CLITERAL(Color){ 243, 147, 51, 255 }
#define OFF_COLOR CLITERAL(Color){ 51, 51, 51, 255 }
#define RECT(x, y, xwidth, y_width) ((Rectangle) { (x), (y), (xwidth), (y_width) })
#define INST_DEBUG_STRING_SIZE 11 // "XXXX = XXXX"

#define COLUMN_WIDTH 270
#define UPPER_MENU_PADDING 30
#define PADDING 17
#define SLIDER_WIDTH 200
#define SLIDER_HEIGHT 20
#define BOX_LEN 20
#define FILENAME_BUFFER_SIZE 512

static int inst_active;
static int inst_focus;
static int inst_scrollIndex;
static char **inst_list;
static bool is_paused;
static bool is_step;
static char bp_text[64];
static bool bp_is_edit_on;
static float audio_freq_slider;
static float audio_volume;
static Quirks quirks;
static List bp_list;

//file dialog
static char file_name_to_read[FILENAME_BUFFER_SIZE];
static bool is_file_selected;
static GuiWindowFileDialogState file_dialog_state;

void draw(IO *io, Regs *regs)
{
    io->display_wait = false;
    io->last_key_pressed = NO_KEY;

    // update
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

    if (file_dialog_state.SelectFilePressed &&
        IsFileExtension(file_dialog_state.fileNameText, ".ch8")) {
        const char *filepath = TextFormat(
            "%s" PATH_SEPARATOR "%s",
            file_dialog_state.dirPathText,
            file_dialog_state.fileNameText
        );
        strncpy(file_name_to_read, filepath, FILENAME_BUFFER_SIZE - 1);

        // TODO consider just using the .SelectFilePressed
        is_file_selected = true;
        file_dialog_state.SelectFilePressed = false;
    }

    BeginDrawing();
        ClearBackground(BLACK);

        DrawTextureEx(
            io->screen_texture,
            (Vector2) { 0, UPPER_MENU_PADDING },
            0.0f,
            (float) SCALE,
            WHITE);

        #ifdef DEBUG
        draw_debug(regs, io->keys_down);
        #endif

        draw_gui(
            PADDING,
            DISPLAY_HEIGHT * SCALE + UPPER_MENU_PADDING + PADDING);

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
    bp_is_edit_on = false;
    audio_freq_slider = 440.0f;
    audio_volume = 7.0f;
    bp_list = list_create(64);
    inst_list = alloc_debugger_inst_array();
    quirks = init_quirks();
    file_dialog_state = InitGuiWindowFileDialog(GetWorkingDirectory());
    file_dialog_state.windowBounds.height = DISPLAY_HEIGHT * SCALE + 100;
    file_dialog_state.windowBounds.y = PADDING;
    is_file_selected = false;
}

void draw_gui(const int gui_x, const int gui_y)
{

    // volume slider
    GuiSlider(
        RECT(gui_x + PADDING, gui_y, SLIDER_WIDTH, SLIDER_HEIGHT),
        "Vol",
        TextFormat("%2.f%%", audio_volume * 100),
        &audio_volume,
        0.0f,
        1.0f);

    // volume Frequency slider
    // converting to frequency cause raygui sliders must be float
    GuiSlider(
        RECT(gui_x + PADDING, gui_y + 40, SLIDER_WIDTH, SLIDER_HEIGHT),
        "Freq",
        TextFormat("%dHz", (int) audio_freq_slider),
        &audio_freq_slider,
        50.0f,
        2000.0f);

    // IPF
    GuiSlider(
        RECT(gui_x + PADDING, gui_y + 80, SLIDER_WIDTH, SLIDER_HEIGHT),
        "IPF",
        TextFormat("%dIPF", (int)quirks.ipf),
        &quirks.ipf,
        1.0f,
        40.0f);

    const int second_row_x = gui_x + COLUMN_WIDTH;
    // Quirks
    GuiCheckBox(
        RECT(second_row_x, gui_y, BOX_LEN, BOX_LEN),
        "display wait",
        &(quirks.is_vblank));
    GuiCheckBox(
        RECT(second_row_x, gui_y + 40, BOX_LEN, BOX_LEN),
        "8xy6 & 8xye: Vx as input and output",
        &quirks.is_shift);
    GuiCheckBox(
        RECT(second_row_x, gui_y + 80, BOX_LEN, BOX_LEN),
        "bxnn instead of bnnn",
        &quirks.is_jump);
    GuiCheckBox(
        RECT(second_row_x, gui_y + 120, BOX_LEN, BOX_LEN),
        "fx55 & fx65: Leave I unchanged",
        &quirks.is_memory_leave_i_unchanged);

    const int third_row_x = second_row_x + COLUMN_WIDTH;

    GuiCheckBox(
        RECT(third_row_x, gui_y, BOX_LEN, BOX_LEN),
        "8xy1 & 8xy2 & 8xy3: reset Vf",
        &quirks.is_logic);

    GuiCheckBox(
        RECT(third_row_x, gui_y + 40, BOX_LEN, BOX_LEN),
        "8xy6 & 8xye: increment by x",
        &quirks.is_memory_increment_by_x);

    GuiCheckBox(
        RECT(third_row_x, gui_y + 80, BOX_LEN, BOX_LEN),
        "logical scrolling in lores",
        &quirks.is_logical_scroll);

    if (file_dialog_state.windowActive) GuiLock();
    if (GuiButton(RECT(0, 0, 140, 30),
        GuiIconText(ICON_FILE_OPEN, "Open rom"))) {
            file_dialog_state.windowActive = true;
    }
    GuiUnlock();
    GuiWindowFileDialog(&file_dialog_state);
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
    Rectangle step_icon = RECT(debug_x + 35, debug_buttons_y, 30, 30);
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
    // lores
    GuiLabel(
        RECT(debug_x + 69 + padding, debug_y + 340 + padding + label_spacing*4,
            label_width, label_height ),
        TextFormat("%s", regs->is_extended_mode ? "Hires" : "Lores"));

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

bool gui_bp_contains(u16 address)
{
    return list_contains(&bp_list, address);
}

int gui_get_ipf()
{
    return (int)quirks.ipf;
}

bool gui_is_paused()
{
    return is_paused;
}

void gui_set_pause(bool value)
{
    is_paused = value;
}

bool gui_is_step()
{
    return is_step;
}

void gui_set_step(bool value)
{
    is_step = value;
}

float gui_get_volume()
{
    return audio_volume;
}

float gui_get_freq()
{
    return audio_freq_slider;
}

Quirks gui_get_quirks()
{
    return quirks;
}

void gui_set_quirks(Quirks value)
{
    quirks = value;
}

char * gui_get_file_name()
{
    return file_name_to_read;
}

bool gui_is_file_dropped()
{
    return is_file_selected;
}

void gui_set_file_dropped(bool value)
{
    is_file_selected = value;
}

void gui_unload()
{
    #ifdef DEBUG
    free_debugger_inst_array();
    #endif
    list_dealloc(&bp_list);
}