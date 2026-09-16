#include "graphics/host_gpu/renderer/masterSemaphore.h"

#include "common/assert.h"
#include "graphics/host_gpu/graphicContext.h"
#include "graphics/host_gpu/renderer/commandScheduler.h"

#include <execinfo.h>

namespace Libs::Graphics {

MasterSemaphore::MasterSemaphore(GraphicContext& graphics): m_graphics(graphics) {
	vk::SemaphoreTypeCreateInfo type_info {};
	type_info.semaphoreType = vk::SemaphoreType::eTimeline;
	type_info.initialValue  = 0;

	vk::SemaphoreCreateInfo create_info {};
	create_info.pNext = &type_info;

	const auto result = m_graphics.device.createSemaphore(&create_info, nullptr, &m_semaphore);
	EXIT_NOT_IMPLEMENTED(result != vk::Result::eSuccess || m_semaphore == nullptr);
}

MasterSemaphore::~MasterSemaphore() {
	if (m_semaphore != nullptr) {
		m_graphics.device.destroySemaphore(m_semaphore, nullptr);
	}
}

void MasterSemaphore::Refresh() {
	uint64_t   counter = 0;
	const auto result  = m_graphics.device.getSemaphoreCounterValue(m_semaphore, &counter);
	EXIT_NOT_IMPLEMENTED(result != vk::Result::eSuccess);

	auto known = m_gpu_tick.load(std::memory_order_acquire);
	while (known < counter &&
	       !m_gpu_tick.compare_exchange_weak(known, counter, std::memory_order_release,
	                                         std::memory_order_relaxed)) {
	}
}

void MasterSemaphore::Wait(uint64_t tick) {
	if (IsFree(tick)) {
		return;
	}
	Refresh();
	if (IsFree(tick)) {
		return;
	}

	vk::SemaphoreWaitInfo wait_info {};
	wait_info.semaphoreCount = 1;
	wait_info.pSemaphores    = &m_semaphore;
	wait_info.pValues        = &tick;

	for (int i = 1;; i++) {
		const auto result = m_graphics.device.waitSemaphores(&wait_info, 1000000000ULL); // 1s
		if (result == vk::Result::eSuccess) {
			break;
		}
		if (result == vk::Result::eTimeout) {
			uint64_t current_val = 0;
			(void)m_graphics.device.getSemaphoreCounterValue(m_semaphore, &current_val);
			LOGF("[MasterSemaphore::Wait] TIMEOUT (%d s) waiting for tick=%" PRIu64
			     ", counter=%" PRIu64 "\n",
			     i, tick, current_val);
			std::printf("[MasterSemaphore::Wait] TIMEOUT (%d s) waiting for tick=%" PRIu64
			            ", counter=%" PRIu64 "\n",
			            i, tick, current_val);
			std::fflush(stdout);
			if (i == 1) {
				void* callstack[32];
				int   frames = backtrace(callstack, 32);
				std::printf("=== MasterSemaphore::Wait CALLSTACK ===\n");
				backtrace_symbols_fd(callstack, frames, fileno(stdout));
				DumpRecentSubmits();
			}
			// TEMP-DIAG-LONGWAIT: 300 s watchdog to distinguish very-slow
			// dispatches (27k-group ballot loops) from infinite ones.
			if (i >= 300) {
				EXIT("[MasterSemaphore::Wait] GPU hang: timed out after %d s waiting for "
				     "tick=%" PRIu64 "\n",
				     i, tick);
			}
			continue;
		}
		EXIT("[MasterSemaphore::Wait] waitSemaphores returned error: %d\n",
		     static_cast<int>(result));
	}
	Refresh();
}

} // namespace Libs::Graphics
