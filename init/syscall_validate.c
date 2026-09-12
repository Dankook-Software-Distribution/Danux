#include <danux/page.h>
#include <danux/syscall_validate.h>
#include <danux/vmm.h>

int validate_user_range(process_t *proc, const void *uptr, size_t len, int write) {
	if (len == 0)
		return 1;

	uint64_t start = (uint64_t)uptr;

	/*
	 * 오버플로 먼저 거른다. 이게 없으면 검증이 통째로 우회된다:
	 * len을 크게 주면 start + len이 한 바퀴 돌아 end가 작아지고,
	 * 그러면 아래 경계 검사(end > USER_STACK_TOP)를 통과하는 데다
	 * 페이지 루프 조건(page < end)도 처음부터 거짓이라 한 번도 돌지 않는다.
	 * 결국 "검사 통과"로 나가고 커널이 버퍼 뒤쪽 메모리를 그대로 읽는다.
	 */
	if (start + len < start)
		return 0;

	uint64_t end = start + len;

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
