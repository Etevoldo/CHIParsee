#pragma once
#include "types.h"

#define B1000_0000 (1 << 7)
#define B0000_0001 (1)

Quirks init_quirks(void);
Regs init_regs(bool isXO);
void FDE(Regs *regs, IO *io, Quirks quirks);
