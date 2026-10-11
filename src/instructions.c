#include <time.h>
#include <stdlib.h>

#include "instructions.h"
#include "stack.h"
#include "renderer.h"

// init default values for quirks
Quirks init_quirks()
{
    return (Quirks) {
        .is_display_wait = true,
        .is_8xy6e_vy_into_vx = true,
        .is_bnnn_vx = false,
        .is_fx565_change_I = false
    };
}

void FDE(Regs *regs, IO *io, Quirks quirks)
{
    // for short
    u16 pc = regs->pc;
    // fetch

    // current instruction
    u16 instruc = (regs->ram[pc] << 8) + regs->ram[pc + 1];
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
        if (other_types == 0x0E0) {
            clear_display(io->display);
        }
        else if (other_types == 0x00EE) {
            regs->pc = stackPop(&(regs->stack));
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
        if (quirks.is_bnnn_vx) { regs->pc = regs->v[n] + kk; }
        else regs->pc = regs->v[0x0] + nnn;
        break;
    case 0xC000:
        srand((unsigned) time(NULL));
        regs->v[x] = rand() & kk;
        break;
    case 0xD000:
        io->display_wait = true;
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
        break;
    }
}

void op_DXYN(u8 x_index, u8 y_index, int n, Regs *regs, bool display[])
{
    const u16 spriteIndex = regs->index;
    const u8 xAnchor = regs->v[x_index] % DISPLAY_WIDTH;
    const u8 yAnchor = regs->v[y_index] % DISPLAY_HEIGHT;
    regs->v[0xF] = 0;

    u8 x = xAnchor;
    u8 y = yAnchor;

    for (int byteN = 0; byteN < n; byteN++) {
        x = xAnchor;
        int mask = 0b10000000; // no digit separator in C :(
        int byte = regs->ram[spriteIndex + byteN];
        for (int bitN = 0; bitN < 8; bitN++) {
            bool isBitOn = byte & mask;
            bool isCollision = drawPixel(x, y, isBitOn, display);

            if (isCollision) { 
                regs->v[0xF] = 1;
            }

            mask = mask >> 1;
            x++;

            if (x >= DISPLAY_WIDTH) break;
        }
        y++;
        if (y >= DISPLAY_HEIGHT) break;
    }
}

void op_8000(
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
        v[0xF] = 0;
        break;
    case 0x0002:
        v[x_index] = v[x_index] & v[y_index];
        v[0xF] = 0;
        break;
    case 0x0003:
        v[x_index] = v[x_index] ^ v[y_index];
        v[0xF] = 0;
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
        if (quirks.is_8xy6e_vy_into_vx) { v[x_index] = v[y_index]; }
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
        if (quirks.is_8xy6e_vy_into_vx) { v[x_index] = v[y_index]; }
        int shifted_bit = (v[x_index] & 0b10000000) >> 7;

        v[x_index] = v[x_index] << 1;

        v[0xF] = (shifted_bit == 1) ? 1 : 0;
        break;
    default:
        break;
    }
}

void op_FX00(u8 x_index, int other_type, Regs *regs, IO *io, Quirks quirks)
{
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
        if (quirks.is_fx565_change_I) {
            for (int i = 0; i < x_index; i++, regs->index++) {
                regs->ram[regs->index] = regs->v[i];
            }
            break;
        }
        for (int i = 0; i <= x_index; i++) {
            regs->ram[regs->index + i] = regs->v[i];
        }
        regs->index += x_index + 1;
        break;
    case 0x0065:
        if (quirks.is_fx565_change_I) {
            for (int i = 0; i <= x_index; i++, regs->index++) {
                regs->v[i] = regs->ram[regs->index];
            }
            break;
        }
        for (int i = 0; i <= x_index; i++) {
            regs->v[i] = regs->ram[regs->index + i];
        }
        regs->index += x_index + 1;
        break;
    default:
        break;
    }
}