#include <danux/page.h>
#include <danux/syscall_validate.h>
#include <danux/vmm.h>

int validate_user_range(process_t *proc, const void *uptr, size_t len, int write) {
	if (len == 0)
		return 1;

	uint64_t start = (uint64_t)uptr;
	uint64_t end = start + len;	// PoC: 오버플로 검사는 생략

	// 이 프로세스의 유저 영역 밖을 통째로 걸러낸다 -- 커널 포인터를 syscall에
	// 넘겨서 커널이 대신 읽고/쓰게 만드는 전형적인 공격을 여기서 막는다.
	if (start < USER_VADDR_BASE || end > USER_STACK_TOP)
		return 0;

	for (uint64_t page = start & ~(uint64_t)(PAGE_SIZE - 1); page < end; page += PAGE_SIZE) {
		uint64_t flags;
		if (!vmm_lookup(proc->pml4, page, &flags))
			return 0;
		if (!(flags & VMM_USER))
			return 0;
		if (write && !(flags & VMM_WRITABLE))
			return 0;
	}

	return 1;
}
