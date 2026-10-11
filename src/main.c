#include <stdio.h>
#include <unistd.h>
#include <math.h>

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "raylib.h"
#include "types.h"
#include "stack.h"
#include "list.h"
#include "renderer.h"
#include "instructions.h"
#include "audio.h"
#include "io.h"
#include "style_amber.h"

#define SAMPLE_RATE 44100
#define BUFFER_SIZE 4096

bool main_cycle(IO *io, Regs *regs, GuiVars *gui_vars);
void close_services(AudioStream stream, 
    Image screen_image,
    Texture screen_texture);
IO init_io();
Regs init_regs();

int main(int argc, char *argv[])
{
    #ifdef DEBUG_ON
    InitWindow(
        DISPLAY_WIDTH * SCALE + 510,
        DISPLAY_HEIGHT * SCALE + 200,
        "Chip-8 Emu");
    #else
    InitWindow(DISPLAY_WIDTH * SCALE,
        DISPLAY_HEIGHT * SCALE,
        "Chip-8 Emu");
    #endif
    GuiLoadStyleAmber();

    Regs regs = init_regs();
    IO io = init_io();
    GuiVars gui_vars = init_gui_vars();
    AudioStream stream = init_audio();
    gui_vars.audio_data = init_audio_vars(stream);
    gui_vars.quirks = init_quirks();
    alloc_debugger_inst_array(&gui_vars.inst_list);

    load_font(regs.ram);
    const int frame_time = 17; // amount of time between frames in miliseconds

    // Insert ROM gui pre-loop
    while (true) {
        if (IsFileDropped()) {
            FilePathList dropped_rom = LoadDroppedFiles();
            if ((dropped_rom.count > 0) &&
                 IsFileExtension(dropped_rom.paths[0], ".ch8")) {
                load_rom(regs.ram, dropped_rom.paths[0]);
                #ifdef DEBUG_ON
                load_debugger_inst_array(regs.ram, gui_vars.inst_list);
                #endif
                break;
            }
        }
        draw(&io, &regs, &gui_vars);
        usleep(frame_time * 1000);

        if (WindowShouldClose()) {
            close_services(stream, io.screen_image, io.screen_texture);
            return 0;
        }
    }

    while (!WindowShouldClose()) {
        if (IsFileDropped()) {
            FilePathList dropped_rom = LoadDroppedFiles();
            if ((dropped_rom.count > 0) &&
                 IsFileExtension(dropped_rom.paths[0], ".ch8")) {
                UnloadTexture(io.screen_texture);
                UnloadImage(io.screen_image);
                regs = init_regs();
                io = init_io();
                load_rom(regs.ram, dropped_rom.paths[0]);

                UnloadDroppedFiles(dropped_rom);

                #ifdef DEBUG_ON
                load_debugger_inst_array(regs.ram, gui_vars.inst_list);
                #endif
            }
        }

        if (!main_cycle(&io, &regs, &gui_vars)) break;

        draw(&io, &regs, &gui_vars);

        usleep(frame_time * 1000);
    }

    #ifdef DEBUG_ON
    free_debugger_inst_array(gui_vars.inst_list);
    #endif

    close_services(stream, io.screen_image, io.screen_texture);

    return 0;
}

void close_services(
    AudioStream stream,
    Image screen_image,
    Texture screen_texture)
{
    CloseAudioDevice();
    UnloadTexture(screen_texture);
    UnloadImage(screen_image);
    UnloadAudioStream(stream);
    CloseWindow();
}

bool main_cycle(IO *io, Regs *regs, GuiVars *gui_vars)
{
    if (IsAudioStreamProcessed(gui_vars->audio_data.stream)) {
        sample_audio_buffer(&gui_vars->audio_data);
        UpdateAudioStream(
            gui_vars->audio_data.stream,
            gui_vars->audio_data.buffer,
            BUFFER_SIZE);
    }

    if (regs->delay > 0) regs->delay -= 1;
    if (regs->sound > 0) regs->sound -= 1;

    // audio
    if (regs->sound == 0) PauseAudioStream(gui_vars->audio_data.stream);
    if (regs->sound) ResumeAudioStream(gui_vars->audio_data.stream);

    handle_input(io->keys_down, &io->last_key_pressed);

    const int IPF = gui_vars->IPF;        // instructions per frame
    for (int i = 0; i < IPF; i++) {
        if (io->display_wait && gui_vars->quirks.is_display_wait) break; 

        while (gui_vars->is_paused) {
            // step 1 instruction
            if (gui_vars->is_step) {
                gui_vars->is_step = false;
                break;
            }
            if (WindowShouldClose()) return false;
            draw(io, regs, gui_vars);
        }

        FDE(regs, io, gui_vars->quirks);
        // pause on breakpoint
        if (list_contains(&gui_vars->bp_list, regs->pc)) {
            gui_vars->is_paused = true;
        }
        update_scroll(regs, gui_vars);
    }
    return true;
}

IO init_io()
{
    IO io = (IO) {
        .display = { false },
        .keys_down = { false },
        .last_key_pressed = NO_KEY,
        .display_wait = false,
        .screen_image = GenImageColor(DISPLAY_WIDTH, DISPLAY_HEIGHT, GREEN),
        .screen_texture = LoadTextureFromImage(io.screen_image)
    };
    return io;
}

Regs init_regs()
{
    return (Regs) {
        .v = { 0 },
        .index = 0,
        .delay = 0,
        .sound = 0,
        .ram = { 0 },
        .stack = (Stack) {0, { 0 }},
        .pc = PC_START,
    };
}
