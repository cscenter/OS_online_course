#include <backtrace.h>
#include <print.h>


void backtrace(uintptr_t rbp, uintptr_t stack_bottom, uintptr_t stack_top)
{
	int frame_index = 0;

	while (rbp >= stack_bottom && rbp + 16 <= stack_top) {
		const uint64_t *frame = (const uint64_t *)rbp;
		const uintptr_t prev_rbp = frame[0];
		const uintptr_t prev_rip = frame[1];

		if (prev_rbp <= rbp)
			break;

		printf("%d: RIP 0x%llx\n", frame_index,
					(unsigned long long)prev_rip);
		rbp = prev_rbp;
		++frame_index;
	}
}
