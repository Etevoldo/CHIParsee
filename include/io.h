#pragma once
#include <stdbool.h>
#include "types.h"

IO init_io();
void handle_input(bool *keys_down, int *last_key_pressed);
void load_font(u8 *ram);
int load_rom(u8 *ram, char *file_name);

bool drawPixel(int x, int y, bool isBitOn, bool display[]);
void clear_display(bool display[]);