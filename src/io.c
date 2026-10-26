#include <stdio.h>
#include "raylib.h"
#include "types.h"
#include "io.h"

static int map_key(int key);
// copies values of display into copy, also clearing it
static void buffer_clear_swap(bool display[], bool copy[]);

IO init_io()
{
    IO io = (IO) {
        .display = { false },
        .keys_down = { false },
        .last_key_pressed = NO_KEY,
        .display_wait = false,
        .screen_image = GenImageColor(DISPLAY_WIDTH, DISPLAY_HEIGHT, GREEN),
        .screen_texture = LoadTextureFromImage(io.screen_image),
    };
    return io;
}

void handle_input(bool *keys_down, int *last_key_pressed)
{
    static int key_codes[N_OF_KEYS] = {
        KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_Q, KEY_W, KEY_E,
        KEY_R, KEY_A, KEY_S, KEY_D, KEY_F, KEY_Z, KEY_X, KEY_C, KEY_V,
    };

    int key_index;
    for (int i = 0; i < N_OF_KEYS; i++) {
        key_index = map_key(key_codes[i]);
        if (IsKeyDown(key_codes[i]) && !keys_down[key_index]) {
            keys_down[key_index] = true;
        }
        if (IsKeyUp(key_codes[i]) && keys_down[key_index]) {
            keys_down[key_index] = false;
            *last_key_pressed = key_index;
        }
    }

}

static int map_key(int key)
{
    switch (key){
        case KEY_ONE:   return 0x1;
        case KEY_TWO:   return 0x2;
        case KEY_THREE: return 0x3;
        case KEY_FOUR:  return 0xC;
        case KEY_Q:     return 0x4;
        case KEY_W:     return 0x5; 
        case KEY_E:     return 0x6; 
        case KEY_R:     return 0xD; 
        case KEY_A:     return 0x7; 
        case KEY_S:     return 0x8; 
        case KEY_D:     return 0x9; 
        case KEY_F:     return 0xE; 
        case KEY_Z:     return 0xA; 
        case KEY_X:     return 0x0; 
        case KEY_C:     return 0xB; 
        case KEY_V:     return 0xF;
        default:        return NO_KEY;
    }
}

int load_rom(u8 *ram, char *file_name)
{
    FILE *rom;
    if ((rom = fopen(file_name, "rb")) == NULL) {
        perror("Could not open file");
        return 1;
    }

    fread(ram + PC_START, 1, RAM_SIZE - PC_START, rom);

    fclose(rom);
    return 0;
}

void load_font(u8 *ram)
{
    static u8 font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
        0x20, 0x60, 0x20, 0x20, 0x70,  // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
        0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
        0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
        0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
        0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
        0xF0, 0x80, 0xF0, 0x80, 0x80}; // F

    const int font_length = sizeof(font) / sizeof(font[0]);

    for (int i = 0; i < font_length; i++) {
        ram[FONT_START + i] = font[i];
    }

    static u8 big_font[] = {
        0x3c, 0x7e, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0x7e, 0x3c, // big 0
        0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, // big 1
        0x3e, 0x7f, 0xc3, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xff, 0xff, // big 2
        0x3c, 0x7e, 0xc3, 0x03, 0x0e, 0x0e, 0x03, 0xc3, 0x7e, 0x3c, // big 3
        0x06, 0x0e, 0x1e, 0x36, 0x66, 0xc6, 0xff, 0xff, 0x06, 0x06, // big 4
        0xff, 0xff, 0xc0, 0xc0, 0xfc, 0xfe, 0x03, 0xc3, 0x7e, 0x3c, // big 5
        0x3e, 0x7c, 0xc0, 0xc0, 0xfc, 0xfe, 0xc3, 0xc3, 0x7e, 0x3c, // big 6
        0xff, 0xff, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x60, 0x60, // big 7
        0x3c, 0x7e, 0xc3, 0xc3, 0x7e, 0x7e, 0xc3, 0xc3, 0x7e, 0x3c, // big 8
        0x3c, 0x7e, 0xc3, 0xc3, 0x7f, 0x3f, 0x03, 0x03, 0x3e, 0x7c  // big 9
    };

    for (size_t i = 0; i < (sizeof(big_font) / sizeof(big_font[0])); i++) {
        ram[BIG_FONT_START + i] = big_font[i];
    }
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

void scroll_left(bool display[], int amount)
{
    bool copy[DISPLAY_HEIGHT * DISPLAY_WIDTH];
    buffer_clear_swap(display, copy);
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            if (x < amount) continue;
            int index = x + y * DISPLAY_WIDTH;
            display[index] = copy[index + amount];
        }
    }
}

void scroll_right(bool display[], int amount)
{
    bool copy[DISPLAY_HEIGHT * DISPLAY_WIDTH];
    buffer_clear_swap(display, copy);
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            if (x + amount >= DISPLAY_WIDTH) continue;
            int index = x + y * DISPLAY_WIDTH;
            display[index + amount] = copy[index];
        }
    }
}

void scroll_down(bool display[], int amount)
{
    bool copy[DISPLAY_HEIGHT * DISPLAY_WIDTH] = { 0 };
    buffer_clear_swap(display, copy);
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            if (y + amount >= DISPLAY_HEIGHT) continue; // could just end it
            int index = x + y * DISPLAY_WIDTH;
            display[index + amount * DISPLAY_WIDTH] = copy[index];
        }
    }
}

static void buffer_clear_swap(bool display[], bool copy[])
{
    for (int i = 0; i < DISPLAY_HEIGHT * DISPLAY_WIDTH; i++) {
        copy[i] = display[i];
        display[i] = false;
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