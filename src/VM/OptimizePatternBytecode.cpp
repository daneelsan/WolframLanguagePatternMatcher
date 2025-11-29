#include "VM/OptimizePatternBytecode.h"
#include "VM/Opcode.h"

namespace PatternMatcher::BytecodeOptimizer
{

/**
 * @brief Remove dead branch instructions
 *
 * Detects pattern where a boolean is set to true (non-zero) and then
 * immediately used in BRANCH_FALSE. Since the branch condition is
 * always false, the branch never executes - remove both instructions.
 *
 * This is SAFE because:
 *   - LOAD_IMM only appears in epilogue blocks (success/fail handlers)
 *   - BRANCH_FALSE immediately follows compiler-generated SAMEQ
 *   - These sequences are atomic and never have labels between them
 *   - The compiler never generates labels pointing to these instructions
 *
 * Pattern: LOAD_IMM %b, <non-zero>
 *          BRANCH_FALSE %b, L
 */
bool eliminateDeadBranches(PatternBytecode& bc)
{
	bool changed = false;
	auto& instrs = bc.getInstructions();

	for (size_t i = 0; i + 1 < instrs.size();)
	{
		// Look for: LOAD_IMM %b, <non-zero> followed by BRANCH_FALSE %b, L
		if (instrs[i].opcode == Opcode::LOAD_IMM && instrs[i + 1].opcode == Opcode::BRANCH_FALSE)
		{
			// Extract operands safely
			auto dstBool = std::get_if<BoolRegOp>(&instrs[i].ops[0]);
			auto immMint = std::get_if<ImmMint>(&instrs[i].ops[1]);
			auto jumpBool = std::get_if<BoolRegOp>(&instrs[i + 1].ops[0]);

			// Check if same register and immediate is non-zero (true)
			if (dstBool && immMint && jumpBool && (dstBool->v == jumpBool->v) && (immMint->v != 0))
			{
				// Dead code: branch never taken
				// Safe to remove - these are compiler-generated sequences that never have labels
				instrs.erase(instrs.begin() + i, instrs.begin() + i + 2);
				changed = true;
				continue; // Check same position again
			}
		}
		++i;
	}

	return changed;
}

}; // namespace PatternMatcher::BytecodeOptimizer
