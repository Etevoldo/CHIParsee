#pragma once
#include "types.h"

Regs init_regs();
void FDE(Regs *regs, IO *io, Quirks quirks);
void op_DXYN(u8 x_index, u8 y_index, int n, Regs *regs, bool display[]);

void op_8000(u8 x_index, u8 y_index, int logical_type,
    Regs *regs, Quirks quirks);

void op_FX00(u8 x_index, int other_type,
    Regs *regs, IO *io, Quirks quirks);

Quirks init_quirks();
