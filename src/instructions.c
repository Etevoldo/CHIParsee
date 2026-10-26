#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "renderer.h"
#include "instructions.h"
#include "io.h"
#include "stack.h"

// op_DXYN helper function
static void draw_byte(
    Regs *regs, bool display[], u8 byte, u8 x, u8 y, int width_cap);
static void op_DXYN(u8 x_index, u8 y_index, int n, Regs *regs, bool display[]);

static void op_8000(u8 x_index, u8 y_index, int logical_type,
    Regs *regs, Quirks quirks);

static void op_FX00(u8 x_index, int other_type,
    Regs *regs, IO *io, Quirks quirks);

// init default values for quirks
Quirks init_quirks()
{
    return (Quirks) {
        .is_shift = false,
        .is_memory_increment_by_x = false,
        .is_memory_leave_i_unchanged = false,
        .is_wrap = false,
        .is_jump = false,
        .is_vblank = true,
        .is_logic = true,
        .ipf = 15.0f
    };
}

Regs init_regs()
{
    Regs regs = (Regs) {
        .v = { 0 },
        .index = 0,
        .delay = 0,
        .sound = 0,
        .ram = { 0 },
        .stack = (Stack) {0, { 0 }},
        .pc = PC_START,
        .is_extended_mode = false
    };
    load_font(regs.ram);
    return regs;
}

void FDE(Regs *regs, IO *io, Quirks quirks)
{
    // dirty uneffective OOB checker
    regs->pc = regs->pc & 0xFFF;
    regs->index = regs->index & 0xFFF;
    // fetch
    // current instruction
    u16 instruc = (regs->ram[regs->pc] << 8) + regs->ram[regs->pc + 1];
    regs->pc += 2;

    // decode
    const u8 x   = (instruc & 0x0F00) >> 8; // 2nd nibble, V selector
    const u8 y   = (instruc & 0x00F0) >> 4; // 3rd nibble, V selector
    const u8 n   = (instruc & 0x000F);      // 4th nibble, 4 bit number
    const u8 kk  = (instruc & 0x00FF);      // 3rd & 4th nibble, 8 bit number
    const int nnn = (instruc & 0x0FFF);      // 2-4th nibble, memory address

    // for general instructions
    const int type = instruc & 0xF000;
    // for 8VX instructions
    const int logical_type = instruc & 0x000F;
    // for E000 and F000
    const int other_types = instruc & 0x00FF;

    // execute
    switch (type) {
    case 0x0000:
        // TODO 
        if (other_types == 0x0E0) {
            clear_display(io->display);
        }
        else if (other_types == 0x00EE) {
            regs->pc = stackPop(&(regs->stack));
        }
        else if (other_types == 0x00FE) {
            clear_display(io->display);
            regs->is_extended_mode = false;
        }
        else if (other_types == 0x00FD) {
            exit(EXIT_SUCCESS);
        }
        else if (other_types == 0x00FF) {
            clear_display(io->display);
            regs->is_extended_mode = true;
        }
        else if ((instruc & 0x00F0) == 0x00C0) {
            if (n == 0) exit(EXIT_SUCCESS); //scroll

            if (regs->is_extended_mode) scroll_down(io->display, n);
            else scroll_down(io->display, n * 2);
        }
        else if (other_types == 0x00FB) { //scroll right
            if (regs->is_extended_mode) scroll_right(io->display, 4);
            else scroll_right(io->display, 8);
        }
        else if (other_types == 0x00FC) { //scroll left
            if (regs->is_extended_mode) scroll_left(io->display, 4);
            else scroll_left(io->display, 8);
        }
        break;
    case 0x1000:
        regs->pc = nnn;
        break;
    case 0x2000:
        stackPush(&(regs->stack), regs->pc);
        regs->pc = nnn;
        break;
    case 0x3000:
        if (regs->v[x] == kk) regs->pc += 2;
        break;
    case 0x4000:
        if (regs->v[x] != kk) regs->pc += 2;
        break;
    case 0x5000:
        if (regs->v[x] == regs->v[y]) regs->pc += 2;
        break;
    case 0x6000:
        regs->v[x] = kk;
        break;
    case 0x7000:
        // check for overflow
        regs->v[x] += kk;
        break;
    case 0x8000:
        op_8000(x, y, logical_type, regs, quirks);
        break;
    case 0x9000:
        if (regs->v[x] != regs->v[y]) regs->pc += 2;
        break;
    case 0xA000:
        regs->index = nnn;
        break;
    case 0xB000:
        if (quirks.is_jump) { regs->pc = regs->v[n] + kk; }
        else regs->pc = regs->v[0x0] + nnn;
        break;
    case 0xC000:
        srand((unsigned) time(NULL));
        regs->v[x] = rand() & kk;
        break;
    case 0xD000:
        if (!regs->is_extended_mode) io->display_wait = true;
        op_DXYN(x, y, n, regs, io->display);
        break;
    case 0xE000:
        int key_to_check;
        if (other_types == 0x009E) {
            key_to_check = regs->v[x] & 0x000F;
            if (io->keys_down[key_to_check])
                regs->pc += 2;
        }
        else if (other_types == 0x00A1){
            key_to_check = regs->v[x] & 0x000F;
            if (!io->keys_down[key_to_check])
                regs->pc += 2;
        }
        break;
    case 0xF000:
        op_FX00(x, other_types, regs, io, quirks);
        break;
    default:
        printf("Unknown/Unimplemented instruction: %04X", instruc);
        break;
    }
}

static void op_DXYN(u8 x_index, u8 y_index, int n, Regs *regs, bool display[])
{
    if (n == 0) n = 16; // DXY0
    const u16 spriteIndex = regs->index;

    int width_cap = DISPLAY_WIDTH;
    int height_cap = DISPLAY_HEIGHT;
    // consider the screen 2x smaller in lores mode
    if (!regs->is_extended_mode) {
        width_cap /= 2;
        height_cap /= 2;
    }
    const u8 xAnchor = regs->v[x_index] % width_cap;
    const u8 yAnchor = regs->v[y_index] % height_cap;
    regs->v[0xF] = 0;

    u8 x = xAnchor;
    u8 y = yAnchor;

    for (int row = 0; row < n; row++) {
        x = xAnchor;
        int byte;
        if ((n == 16) && regs->is_extended_mode) {
            byte = regs->ram[spriteIndex + row*2];
            draw_byte(regs, display, byte, x, y, width_cap);
            byte = regs->ram[spriteIndex + row*2 + 1];
            draw_byte(regs, display, byte, x+8, y, width_cap);
        }
        else {
            byte = regs->ram[spriteIndex + row];
            draw_byte(regs, display, byte, x, y, width_cap);
        }
        y++;
        if (y >= height_cap) break;
    }
}

static void draw_byte(
    Regs *regs, bool display[], u8 byte, u8 x, u8 y, int width_cap)
{
    int mask = 0b10000000; // no digit separator in C :(
    for (int bitN = 0; bitN < 8; bitN++) {
        bool isBitOn = byte & mask;

        bool isCollision = false;
        if (!regs->is_extended_mode) {
            // 2x2 pixel
            isCollision = drawPixel(x*2  , y*2,   isBitOn, display);
            isCollision = drawPixel(x*2+1, y*2,   isBitOn, display);
            isCollision = drawPixel(x*2  , y*2+1, isBitOn, display);
            isCollision = drawPixel(x*2+1, y*2+1, isBitOn, display);
        }
        else {
            isCollision = drawPixel(x, y, isBitOn, display);
        }

        if (isCollision) { 
            regs->v[0xF] = 1;
        }

        mask = mask >> 1;
        x++;

        if (x >= width_cap) return;
    }
}

static void op_8000(
    u8 x_index, u8 y_index, int logical_type,
    Regs *regs, Quirks quirks)
{
    int sub, sum, shifted_bit;
    u8 *v = regs->v;
    switch (logical_type) {
    case 0x0000:
        v[x_index] = v[y_index];
        break;
    case 0x0001:
        v[x_index] = v[x_index] | v[y_index];
        if (quirks.is_logic) v[0xF] = 0;
        break;
    case 0x0002:
        v[x_index] = v[x_index] & v[y_index];
        if (quirks.is_logic) v[0xF] = 0;
        break;
    case 0x0003:
        v[x_index] = v[x_index] ^ v[y_index];
        if (quirks.is_logic) v[0xF] = 0;
        break;
    case 0x0004:
        sum = v[x_index] + v[y_index];
        v[x_index] += v[y_index];

        v[0xF] = (sum > 0xFF) ? 1 : 0;
        break;
    case 0x0005:
        sub = v[x_index] - v[y_index];
        v[x_index] -= v[y_index];

        v[0xF] = (sub < 0) ? 0 : 1;
        break;
    case 0x0006:
        if (!quirks.is_shift) { v[x_index] = v[y_index]; }
        shifted_bit = v[x_index] & 0b00000001;

        v[x_index] = v[x_index] >> 1;

        v[0xF] = (shifted_bit == 1) ? 1 : 0;
        break;
    case 0x0007:
        int sub = v[y_index] - v[x_index];
        v[x_index] = v[y_index] - v[x_index];

        v[0xF] = (sub < 0) ? 0 : 1;
        break;
    case 0x000E:
        if (!quirks.is_shift) { v[x_index] = v[y_index]; }
        int shifted_bit = (v[x_index] & 0b10000000) >> 7;

        v[x_index] = v[x_index] << 1;

        v[0xF] = (shifted_bit == 1) ? 1 : 0;
        break;
    default:
        break;
    }
}

static void op_FX00(
    u8 x_index, int other_type, Regs *regs, IO *io, Quirks quirks)
{
    static u8 storage[8] = {0};
    switch (other_type) {
    case 0x0007:
        regs->v[x_index] = regs->delay;
        break;
    case 0x000A:
        if (io->last_key_pressed != NO_KEY) {
            regs->v[x_index] = io->last_key_pressed;
        }
        else {
            regs->pc -= 2;
        }
        break;
    case 0x0015:
        regs->delay = regs->v[x_index];
        break;
    case 0x0018:
        regs->sound = regs->v[x_index];
        break;
    case 0x001E:
        regs->index += regs->v[x_index];
        break;
    case 0x0029:
        // big font load
        if ( 15 < regs->v[x_index] && regs->v[x_index] < 26) {
            int big_index = regs->v[x_index] - 15;
            regs->index = BIG_FONT_START + (big_index * 8);
        }
        // regular font
        u16 last_nibble = regs->v[x_index] & 0x0F;
        regs->index = FONT_START + (last_nibble * 5);
        break;
    case 0x0033:
        u8 number = regs->v[x_index];
        u8 digits[3] = { 0 };

        for (int i = 0; i < 3; i++) {
            digits[i] = number % 10;
            number /= 10;
        }

        for (int i = 2; i >= 0; i--) {
            regs->ram[regs->index + i] = digits[2 - i];
        }
        break;
    case 0x0055:
        for (int i = 0; i <= x_index; i++) {
            regs->ram[regs->index + i] = regs->v[i];
        }
        if (quirks.is_memory_leave_i_unchanged) break;

        if (quirks.is_memory_increment_by_x) {
            regs->index += x_index;
        }
        else {
            regs->index += x_index + 1;
        }
        break;
    case 0x0065:
        for (int i = 0; i <= x_index; i++) {
            regs->v[i] = regs->ram[regs->index + i];
        }
        if (quirks.is_memory_leave_i_unchanged) break;

        if (quirks.is_memory_increment_by_x) {
            regs->index += x_index;
        }
        else {
            regs->index += x_index + 1;
        }
        break;
    case 0x0075:
        for (int i = 0; i < x_index; i++) {
            storage[i] = regs->v[i];
        }
        break;
    case 0x0085:
        for (int i = 0; i < x_index; i++) {
            regs->v[i] = storage[i];
        }
        break;
    default:
        printf("Unknown/Unimplemented instruction: F%01X%02X",
            x_index, other_type);
        break;
    }
}