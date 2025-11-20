#pragma once

#include "VM/PatternBytecode.h"

namespace PatternMatcher
{

/// @brief Bytecode optimization passes for pattern matching bytecode
namespace BytecodeOptimizer
{
	/// @brief Remove dead branch instructions that can never execute
	/// @note This is SAFE because LOAD_IMM + BRANCH_FALSE sequences are
	///       compiler-generated and never have labels pointing to them
	/// @note Pattern: LOAD_IMM %b, <non-zero>; BRANCH_FALSE %b, L
	bool eliminateDeadBranches(PatternBytecode& bc);

} // namespace BytecodeOptimizer

} // namespace PatternMatcher
