#include <stdio.h>
#include "raylib.h"
#include "types.h"
#include "io.h"

static int map_key(int key);

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
    if ((rom = fopen(file_name, "rb")) == NULL)
        return 0;

    fread(ram + PC_START, 1, RAM_SIZE - PC_START, rom);

    fclose(rom);
    return 1;
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
}