#pragma once

#include <cstdint>
#include <string_view>

namespace Libs::Graphics {

struct ComputeShaderQuirk {
	std::string_view title_id;     // PSN Title ID (e.g. "PPSA03685")
	const char*      game_name;    // Human-readable game name
	uint64_t         shader_hash;  // Declared AGC shader hash
	const char*      description;  // Explanation of the shader algorithm and deadlock
};

// Returns true if the compute shader implements an inter-workgroup spinlock (such as
// single-pass decoupled look-back prefix sum) that deadlocks on host GPUs when workgroup
// count exceeds resident SM/CU capacity.
[[nodiscard]] bool ShouldClampComputeWorkgroups(uint64_t shader_hash);

} // namespace Libs::Graphics
