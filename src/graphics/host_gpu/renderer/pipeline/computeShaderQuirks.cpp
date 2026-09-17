#include "graphics/host_gpu/renderer/pipeline/computeShaderQuirks.h"

namespace Libs::Graphics {

namespace {

// Table of known compute shaders with inter-workgroup spinlocks that cause forward-progress
// deadlocks on host GPUs where workgroup count exceeds physical SM/CU occupancy (e.g. NVIDIA 36 SMs).
// Clamping group count to 1x1x1 allows the first workgroup to finish without waiting on predecessors.
constexpr ComputeShaderQuirk kOccupancyDeadlockQuirks[] = {
    {
        .title_id    = "PPSA03685",
        .game_name   = "Ghostrunner",
        .shader_hash = 0xdc720ea9efec7946ULL,
        .description = "Tile decoupled look-back prefix sum (submits with 64 workgroups)",
    },
    {
        .title_id    = "PPSA03685",
        .game_name   = "Ghostrunner",
        .shader_hash = 0x600a4464295efa6aULL,
        .description = "Work distribution look-back prefix sum (submits with 36-42 workgroups)",
    },
    {
        .title_id    = "PPSA03685",
        .game_name   = "Ghostrunner",
        .shader_hash = 0xd057bd7084a4492aULL,
        .description = "Ray-tracing BVH traversal megadispatch",
    },
};

} // namespace

bool ShouldClampComputeWorkgroups(uint64_t shader_hash) {
	for (const auto& quirk: kOccupancyDeadlockQuirks) {
		if (quirk.shader_hash == shader_hash) {
			return true;
		}
	}
	return false;
}

} // namespace Libs::Graphics
