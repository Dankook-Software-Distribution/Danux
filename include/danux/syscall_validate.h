#ifndef DANUX_SYSCALL_VALIDATE_H
#define DANUX_SYSCALL_VALIDATE_H

#include <danux/process.h>
#include <stddef.h>
#include <stdint.h>

// [uptr, uptr+len) 전체가 proc의 주소 공간에서 PRESENT|USER로 매핑돼 있으면
// (write가 참이면 WRITABLE도 추가로) 1을, 아니면 0을 반환한다. 유저 포인터를
// 만지는 모든 syscall 핸들러는 역참조 전에 반드시 이걸 거쳐야 한다 -- 안 그러면
// 악의적인/버그 있는 프로세스가 커널 주소(또는 매핑 안 된 주소)를 건네서
// 커널이 ring3 권한으로 읽고/쓰고/망가뜨리게 만들 수 있다.
extern int validate_user_range(process_t *proc, const void *uptr, size_t len, int write);

#endif
