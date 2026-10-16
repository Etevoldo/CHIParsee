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

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define SCALE 15
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
    bool is_display_wait;
    bool is_8xy6e_vy_into_vx;
    bool is_bxnn;
    bool is_fx565_change_I;
} Quirks;

typedef struct {
    AudioStream stream;
    float buffer[BUFFER_SIZE];
    int sample_step;
    float volume;
    int frequency;
} Audio_data;
