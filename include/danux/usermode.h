#ifndef DANUX_USERMODE_H
#define DANUX_USERMODE_H

#include <stdint.h>

// ring0에서 ring3로 iretq를 통해 진입하며 돌아오지 않는다. entry에서 실행이
// 시작되고, RSP=user_stack, GDT_USER_CODE/DATA 셀렉터로 동작한다.
extern void enter_usermode(uint64_t entry, uint64_t user_stack) __attribute__((noreturn));

#endif
