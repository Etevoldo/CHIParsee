#pragma once
#include "types.h"

Quirks init_quirks();
Regs init_regs(bool isXO);
void FDE(Regs *regs, IO *io, Quirks quirks);
