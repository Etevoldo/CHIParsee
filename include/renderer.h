#pragma once
#include "types.h"

#define INST_TO_DISPLAY RAM_SIZE / 2

void init_gui();

// Draw regular screen gui
void draw(IO *io, Regs *regs); 
// Draw debug gui
void draw_gui(Regs *regs);
// Draw debug gui
void draw_debug(Regs *regs, bool keys_down[]);

bool drawPixel(int x, int y, bool isBitOn, bool display[]);
void clear_display(bool display[]);

char ** alloc_debugger_inst_array();
void load_debugger_inst_array(u8 *ram);
void free_debugger_inst_array();

void update_scroll(u16 pc);