#include <danux/gdt.h>
#include <danux/list.h>
#include <danux/process.h>
#include <danux/scheduler.h>
#include <danux/syscall.h>
#include <danux/timer.h>
#include <danux/vmm.h>

// run queue의 센티널 head. scheduler_add()는 이 앞(=꼬리)에 새 프로세스를 넣는다.
static struct list_head run_queue;

process_t *current_process = 0;

void scheduler_init(void) {
	list_head_init(&run_queue);
	register_interrupt_handler(32, scheduler_tick);	// timer.c의 기본 핸들러를 덮어씀
}

void scheduler_add(process_t *proc) {
	list_add_prev(&run_queue, &proc->linkage);

	if (!current_process)
		current_process = proc;
}

// proc 다음 프로세스를 돌려준다. run_queue는 센티널이 하나 끼어있는 원형
// 리스트라, 센티널을 만나면 한 칸 더 건너뛴다.
static process_t *next_in_ring(process_t *proc) {
	struct list_head *node = proc->linkage.next;
	if (node == &run_queue)
		node = node->next;
	return list_entry(node, process_t, linkage);
}

static process_t *pick_next(void) {
	process_t *candidate = next_in_ring(current_process);

	// 최대 한 바퀴만 돌아본다. 실행 가능한 게 없으면 현재 프로세스를 그대로 유지.
	for (int i = 0; i < 64 && candidate->state != PROC_READY; i++) {
		if (candidate == current_process) break;
		candidate = next_in_ring(candidate);
	}
	return candidate;
}

void scheduler_tick(registers_t *regs) {
	timer_handler(regs);	// timer_ticks는 계속 증가시켜야 함(원래 IRQ0 핸들러를 이걸로 덮어썼으므로)

	if (!current_process)
		return;

	process_t *next = pick_next();
	if (next == current_process)
		return;

	process_t *prev = current_process;
	if (prev->state == PROC_RUNNING)
		prev->state = PROC_READY;

	current_process = next;
	next->state = PROC_RUNNING;

	tss_set_kernel_stack(next->kernel_stack_top);
	syscall_set_kernel_stack(next->kernel_stack_top);
	vmm_switch_address_space(next->pml4);

	// prev가 다시 스케줄될 때까지 여기로 안 돌아온다 -- scheduler.h/process.c의
	// 저장된 스택 핸드오프 방식 참고.
	context_switch(&prev->saved_rsp, next->saved_rsp);
}
