#include <stdio.h>
#include <unistd.h>
#include <math.h>


#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#undef RAYGUI_IMPLEMENTATION

#define GUI_WINDOW_FILE_DIALOG_IMPLEMENTATION
#include "gui_window_file_dialog.h"
#undef RAYGUI_IMPLEMENTATION

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

bool main_cycle(IO *io, Regs *regs, Audio_data *audio_data);
void close_services(AudioStream stream,
    Image screen_image,
    Texture screen_texture);

int main(int argc, char *argv[])
{
    #ifdef DEBUG
    InitWindow(
        DISPLAY_WIDTH * SCALE + 510,
        DISPLAY_HEIGHT * SCALE + 200,
        "Chip-8 Emu");
    #else
    InitWindow(
        DISPLAY_WIDTH * SCALE,
        DISPLAY_HEIGHT * SCALE + 200,
        "Chip-8 Emu");
    #endif
    GuiLoadStyleAmber();

    Regs regs = init_regs();
    IO io = init_io();
    init_gui();
    AudioStream stream = init_audio();
    Audio_data audio_data = init_audio_vars(stream);

    load_font(regs.ram);
    const int frame_time = 17; // amount of time between frames in miliseconds

    // Insert ROM gui pre-loop
    while (true) {
        if (gui_is_file_dropped()) {
            break;
        }

        draw(&io, &regs);
        usleep(frame_time * 1000);

        if (WindowShouldClose()) {
            close_services(stream, io.screen_image, io.screen_texture);
            return 0;
        }
    }

    while (!WindowShouldClose()) {
        if (gui_is_file_dropped()) {
            UnloadTexture(io.screen_texture);
            UnloadImage(io.screen_image);
            regs = init_regs();
            io = init_io();
            if (load_rom(regs.ram, gui_get_file_name())) {
                gui_set_file_dropped(false);
            }
            load_font(regs.ram);

            gui_set_file_dropped(false);

            #ifdef DEBUG
            load_debugger_inst_array(regs.ram);
            #endif
        }

        if (!main_cycle(&io, &regs, &audio_data)) break;

        draw(&io, &regs);
        // update audio and quirks config
        audio_data.frequency = (int) gui_get_freq();
        audio_data.volume = gui_get_volume();

        usleep(frame_time * 1000);
    }

    unload_gui();
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

bool main_cycle(IO *io, Regs *regs, Audio_data *audio_data)
{
    if (IsAudioStreamProcessed(audio_data->stream)) {
        sample_audio_buffer(audio_data);
        UpdateAudioStream(
            audio_data->stream,
            audio_data->buffer,
            BUFFER_SIZE);
    }

    if (regs->delay > 0) regs->delay -= 1;
    if (regs->sound > 0) regs->sound -= 1;

    // audio
    if (regs->sound == 0) PauseAudioStream(audio_data->stream);
    if (regs->sound) ResumeAudioStream(audio_data->stream);

    handle_input(io->keys_down, &io->last_key_pressed);

    Quirks quirks = gui_get_quirks();
    const int IPF = gui_get_ipf();        // instructions per frame
    for (int i = 0; i < IPF; i++) {
        if (io->display_wait && quirks.is_display_wait) break; 

        while (gui_is_paused()) {
            // step 1 instruction
            if (gui_is_step()) {
                gui_set_step(false);
                break;
            }
            if (WindowShouldClose()) return false;

            draw(io, regs);
            // update audio and quirks config
            quirks = gui_get_quirks();
            audio_data->frequency = (int) gui_get_freq();
            audio_data->volume = gui_get_volume();
        }

        FDE(regs, io, quirks);
        // pause on breakpoint
        if (gui_bp_contains(regs->pc)) {
            gui_set_pause(true);
        }
        update_scroll(regs->pc);
    }
    return true;
}
