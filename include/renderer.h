#pragma once
#include "types.h"

#define INST_TO_DISPLAY RAM_SIZE / 2

void init_gui();

// Draw regular screen gui
void draw(IO *io, Regs *regs); 
// Draw debug gui
void draw_gui(const int gui_x, const int gui_y);
// Draw debug gui
void draw_debug(Regs *regs, bool keys_down[]);

char ** alloc_debugger_inst_array();
void load_debugger_inst_array(u8 *ram);
void free_debugger_inst_array();

void update_scroll(u16 pc);

bool gui_bp_contains(u16 address);
int gui_get_ipf();
bool gui_is_paused();
void gui_set_pause(bool value);
bool gui_is_step();
void gui_set_step(bool value);
float gui_get_volume();
float gui_get_freq();
Quirks gui_get_quirks();
char * gui_get_file_name();
void gui_set_file_name();
bool gui_is_file_dropped();
void gui_set_file_dropped(bool value);

void gui_unload();