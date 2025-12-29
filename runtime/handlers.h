#ifndef C32_HANDLERS_H_
#define C32_HANDLERS_H_

#include "core32.h"

c32_Long c32_id(c32_Byte* name);
c32_Byte* c32_safeGetBytes(CORE32* vm, c32_Long address);

void c32_registerBaseHandlers(CORE32* vm);

#endif