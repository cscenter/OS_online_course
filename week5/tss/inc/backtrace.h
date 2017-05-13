#ifndef __BACKTRACE_H__
#define __BACKTRACE_H__


#include <stdint.h>


void backtrace(uintptr_t rbp, uintptr_t stack_bottom, uintptr_t stack_top);

#endif /*__BACKTRACE_H__*/
