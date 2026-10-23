#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "raylib.h"
#include "list.h"

typedef uint8_t u8;
typedef uint16_t u16;

typedef struct {
    u8  index;
    u16 arr[16];
} Stack;

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define SCALE 7
#define PC_START 0x200
#define FONT_START 0x055
#define RAM_SIZE 4096
#define V_QTY 16
#define N_OF_KEYS 16
#define NO_KEY 16
#define SAMPLE_RATE 44100
#define BUFFER_SIZE 4096

typedef struct {
    u8    v[V_QTY];
    u16   index;
    u8    delay;
    u8    sound;
    u8    ram[RAM_SIZE];
    Stack stack;
    u16   pc;
    bool  is_extended_mode;
} Regs;

typedef struct {
    Image screen_image;
    Texture screen_texture;
    bool  display[DISPLAY_HEIGHT * DISPLAY_WIDTH];
    bool  keys_down[N_OF_KEYS];
    int   last_key_pressed;
    bool  display_wait;
} IO;

typedef struct {
    bool is_shift;
    bool is_memory_increment_by_x;
    bool is_memory_leave_i_unchanged;
    bool is_wrap;
    bool is_jump;
    bool is_vblank;
    bool is_logic;
    float ipf;
} Quirks;

typedef struct {
    AudioStream stream;
    float buffer[BUFFER_SIZE];
    int sample_step;
    float volume;
    int frequency;
} Audio_data;
