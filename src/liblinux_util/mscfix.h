#pragma once

#ifdef _MSC_VER
// try include_next
//#include_next <stdbool.h>
//#include_next <alloca.h>

// fix macro
#undef __cplusplus
#define __builtin_alloca NULL
#endif
