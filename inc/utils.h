#pragma once

#define BIT_IS_SET(reg, pos)      ( (reg) & (1 << (pos)) )
#define BIT_IS_CLEAR(reg, pos)    ( !( (reg) & (1 << (pos)) )  )
