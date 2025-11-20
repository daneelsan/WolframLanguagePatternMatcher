/*===========================================================================
PatternBytecode.cpp - Bytecode Container and Disassembly

This file implements the PatternBytecode class, which holds compiled pattern
matching bytecode and provides various output formats:

1. toString()    - Compact format for testing and debugging
2. disassemble() - Rich format with indentation, colors, and statistics

The disassembly format is designed to be human-readable with:
- Indentation showing block nesting structure
- Inline jump target annotations (→ L7)
- Label markers (L0:, L1:, etc.) at jump destinations
- Comprehensive statistics footer
===========================================================================*/

#include "VM/PatternBytecode.h"
#include "VM/Opcode.h"

#include "AST/MExpr.h"
#include "AST/MExprPatternTools.h"

#include "Embeddable.h"
#include "Expr.h"
#include "Logger.h"

#include <iomanip>
#include <initializer_list>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace PatternMatcher
{
/*===========================================================================
 Helper Functions
===========================================================================*/

/**
 * @brief Convert a single instruction to string format
 *
 * Output format: "OPCODE          operand1, operand2, ..."
 * The opcode is left-aligned and padded to 16 characters for readability.
 *
 * @param instr The instruction to convert
 * @return String representation of the instruction
 */
std::string instructionToString(const PatternBytecode::Instruction& instr)
{
	std::stringstream ss;

	// Opcode name (padded to 16 chars for alignment)
	const char* name = opcodeName(instr.opcode);
	ss << std::left << std::setw(16) << name;

	// Operands (comma-separated)
	bool first = true;
	for (const auto& op : instr.ops)
	{
		if (!first)
			ss << ", ";
		first = false;
		ss << operandToString(op);
	}
	return ss.str();
}

/*===========================================================================
 Compact Output Format (toString)
===========================================================================*/

/**
 * @brief Generate compact bytecode listing (for tests and simple debugging)
 *
 * Output format:
 *   L0:
 *   0    OPCODE   operands
 *   1    OPCODE   operands
 *   ...
 *
 * This format is intentionally minimal - no indentation, no statistics beyond
 * register counts. Used primarily in test assertions where brevity matters.
 *
 * @return Compact string representation of the bytecode
 */
std::string PatternBytecode::toString() const
{
	std::stringstream ss;

	// Build reverse label map (pc -> label) for displaying jump targets
	std::unordered_map<size_t, Label> pcToLabel;
	for (const auto& [label, pc] : labelMap)
		pcToLabel[pc] = label;

	// Calculate maximum PC width for alignment
	size_t maxPCWidth = std::to_string(instrs.size() - 1).length();

	// Print instructions (compact format - no indentation, no annotations)
	for (size_t pc = 0; pc < instrs.size(); ++pc)
	{
		const auto& instr = instrs[pc];

		// Show label marker if this PC is a jump target
		auto labelIt = pcToLabel.find(pc);
		if (labelIt != pcToLabel.end())
		{
			ss << "\nL" << labelIt->second << ":\n";
		}

		// PC and instruction on a single line
		ss << std::setw(maxPCWidth) << pc << "    ";
		ss << instructionToString(instr);
		ss << "\n";
	}

	// Minimal footer - just register counts
	ss << "\n";
	ss << "----------------------------------------\n";
	ss << "Expr registers: " << exprRegisterCount << ", Bool registers: " << boolRegisterCount << "\n";

	return ss.str();
}

/*===========================================================================
 Rich Disassembly Format (disassemble)
===========================================================================*/

/**
 * @brief Generate rich, human-readable bytecode disassembly
 *
 * Output format:
 *   L0:
 *    0  BEGIN_BLOCK    Label[0]
 *    1    MATCH_HEAD     %e0, Expr[f], Label[4]
 *    2    MOVE           %e1, %e0
 *    3    END_BLOCK      Label[0]
 *
 *   Statistics:
 *     Instructions:      26
 *     Labels:            10
 *     ...
 *
 * Features:
 * - Block structure shown via 2-space indentation per nesting level
 * - BEGIN_BLOCK and END_BLOCK at same indentation (outer level)
 * - Jump targets annotated inline (→ L7)
 * - Label markers (L0:, L1:) at all jump destinations
 * - Comprehensive statistics footer
 * - Optional lexical bindings map
 *
 * This format is designed for:
 * - Interactive debugging in notebooks (with syntax highlighting)
 * - Understanding control flow and block structure
 * - Analyzing bytecode optimization opportunities
 *
 * @return Formatted disassembly string
 */
std::string PatternBytecode::disassemble() const
{
	std::stringstream ss;

	// Build reverse label map (pc -> label) for displaying jump targets
	std::unordered_map<size_t, Label> pcToLabel;
	for (const auto& [label, pc] : labelMap)
		pcToLabel[pc] = label;

	/*-----------------------------------------------------------------------
	  Pass 1: Calculate Indentation Depth

	  Goal: Assign each instruction an indentation level based on block nesting.

	  Strategy:
	  - BEGIN_BLOCK and END_BLOCK appear at the OUTER level
	  - Instructions BETWEEN them are indented one level deeper
	  - We process END_BLOCK first to decrement before assigning depth
	  - We process BEGIN_BLOCK last to increment after assigning depth
	-----------------------------------------------------------------------*/
	int currentBlockDepth = 0;
	std::unordered_map<size_t, int> pcToDepth;

	for (size_t pc = 0; pc < instrs.size(); ++pc)
	{
		const auto& instr = instrs[pc];

		// END_BLOCK: decrement BEFORE assigning (so it appears at outer level)
		if (instr.opcode == Opcode::END_BLOCK && currentBlockDepth > 0)
			currentBlockDepth--;

		// Assign current depth to this instruction
		pcToDepth[pc] = currentBlockDepth;

		// BEGIN_BLOCK: increment AFTER assigning (so next instruction is indented)
		if (instr.opcode == Opcode::BEGIN_BLOCK)
			currentBlockDepth++;
	}

	// Calculate maximum PC width for alignment (e.g., "100" needs width 3)
	size_t maxPCWidth = std::to_string(instrs.size() - 1).length();

	/*-----------------------------------------------------------------------
	  Pass 2: Print Instructions with Statistics Gathering

	  We output each instruction while simultaneously collecting statistics
	  about the bytecode structure (blocks, jumps, backtrack points, etc.)
	-----------------------------------------------------------------------*/
	int blockCount = 0; // Total BEGIN_BLOCK instructions
	int maxBlockDepth = 0; // Maximum nesting level observed
	int jumpCount = 0; // JUMP + BRANCH_FALSE instructions
	int backtrackPoints = 0; // TRY instructions (choice points)

	for (size_t pc = 0; pc < instrs.size(); ++pc)
	{
		const auto& instr = instrs[pc];
		int depth = pcToDepth[pc];
		maxBlockDepth = std::max(maxBlockDepth, depth);

		// Update statistics as we encounter relevant instructions
		if (instr.opcode == Opcode::BEGIN_BLOCK)
			blockCount++;
		if (instr.opcode == Opcode::JUMP || instr.opcode == Opcode::BRANCH_FALSE)
			jumpCount++;
		if (instr.opcode == Opcode::TRY)
			backtrackPoints++;

		// Show label marker if this PC is a jump target (L0:, L1:, etc.)
		auto labelIt = pcToLabel.find(pc);
		if (labelIt != pcToLabel.end())
		{
			ss << "L" << labelIt->second << ":\n";
		}

		// Print PC (program counter) with padding for alignment
		ss << std::setw(maxPCWidth) << pc << "    ";

		// Add indentation: 2 spaces per nesting level
		for (int i = 0; i < depth; ++i)
			ss << "  ";

		// Print the instruction (opcode + operands)
		ss << instructionToString(instr);

		// Add inline annotation for control flow: "→ L7" shows jump target
		if (instr.opcode == Opcode::JUMP || instr.opcode == Opcode::BRANCH_FALSE)
		{
			// Find the Label operand and show its destination
			for (const auto& op : instr.ops)
			{
				if (auto* labelOp = std::get_if<LabelOp>(&op))
				{
					ss << "  → L" << labelOp->v;
					break;
				}
			}
		}

		ss << "\n";
	}

	/*-----------------------------------------------------------------------
	  Statistics Footer

	  Provides a comprehensive summary of the bytecode structure:
	  - Total instruction count
	  - Number of labels (jump targets)
	  - Register usage (both expression and boolean registers)
	  - Block nesting statistics
	  - Control flow complexity (jumps, backtrack points)
	  - Optional: lexical variable bindings (pattern variables → registers)
	-----------------------------------------------------------------------*/
	ss << "\n";
	ss << "========================================\n";
	ss << "Statistics:\n";
	ss << "  Instructions:      " << instrs.size() << "\n";
	ss << "  Labels:            " << labelMap.size() << "\n";
	ss << "  Expr registers:    " << exprRegisterCount << "\n";
	ss << "  Bool registers:    " << boolRegisterCount << "\n";
	ss << "  Blocks:            " << blockCount << " (max depth: " << maxBlockDepth << ")\n";
	ss << "  Jumps:             " << jumpCount << "\n";
	ss << "  Backtrack points:  " << backtrackPoints << "\n";

	// Show lexical bindings if any (pattern variables like x_, y_ → registers)
	if (!lexicalMap.empty())
	{
		ss << "\nLexical bindings:\n";
		for (const auto& [name, reg] : lexicalMap)
		{
			ss << "  " << std::setw(12) << std::left << name << " → %e" << reg << "\n";
		}
	}

	return ss.str();
}

/*===========================================================================
 Statistics Query Methods
===========================================================================*/

int PatternBytecode::getBlockCount() const
{
	int count = 0;
	for (const auto& instr : instrs)
	{
		if (instr.opcode == Opcode::BEGIN_BLOCK)
			count++;
	}
	return count;
}

// Scans all instructions to find the deepest nesting level of BEGIN_BLOCK/END_BLOCK pairs.
int PatternBytecode::getMaxBlockDepth() const
{
	int currentDepth = 0;
	int maxDepth = 0;
	for (const auto& instr : instrs)
	{
		if (instr.opcode == Opcode::END_BLOCK && currentDepth > 0)
			currentDepth--;
		maxDepth = std::max(maxDepth, currentDepth);
		if (instr.opcode == Opcode::BEGIN_BLOCK)
			currentDepth++;
	}
	return maxDepth;
}

// Includes both unconditional jumps (JUMP) and conditional jumps (BRANCH_FALSE).
int PatternBytecode::getJumpCount() const
{
	int count = 0;
	for (const auto& instr : instrs)
	{
		if (instr.opcode == Opcode::JUMP || instr.opcode == Opcode::BRANCH_FALSE)
			count++;
	}
	return count;
}

// TRY instructions create choice points for backtracking in pattern matching.
int PatternBytecode::getBacktrackPointCount() const
{
	int count = 0;
	for (const auto& instr : instrs)
	{
		if (instr.opcode == Opcode::TRY)
			count++;
	}
	return count;
}

// Returns a mapping of pattern variable names (like "x", "y") to their assigned expression register indices.
Expr PatternBytecode::getLexicalBindings() const
{
	if (lexicalMap.empty())
	{
		return Expr::ToExpression("<||>");
	}
	Expr res = Expr::createNormal(lexicalMap.size(), "Association");
	mint i = 1;
	for (const auto& [name, reg] : lexicalMap)
	{
		res.setPart(i++, Expr::construct("Rule", Expr(name), Expr(static_cast<mint>(reg))));
	}
	return res;
}

/*===========================================================================
 Label Resolution
===========================================================================*/

/**
 * @brief Resolve a label to its program counter (PC) address
 *
 * Labels are symbolic names for instruction addresses used in jumps.
 * This function converts a Label to its actual PC value.
 *
 * @param L The label to resolve
 * @return PC address if found, std::nullopt otherwise
 * @throws std::nullopt if label not found (unusual - normally returns optional)
 */
std::optional<size_t> PatternBytecode::resolveLabel(Label L) const
{
	auto it = labelMap.find(L);
	if (it != labelMap.end())
		return it->second;
	throw std::nullopt;
}

/*===========================================================================
 Bytecode Optimization
===========================================================================*/

/**
 * @brief Optimize bytecode by removing dead code and redundant instructions
 *
 * Current optimizations:
 *
 * Pass 1: Dead Branch Elimination
 *   Pattern: LOAD_IMM %b, 1
 *            BRANCH_FALSE %b, L
 *
 *   Since %b is always true (1), the BRANCH_FALSE never executes.
 *   We remove both instructions.
 *
 * Future optimizations:
 *   - Unreachable code after unconditional jumps
 *   - Redundant MOVE elimination
 *   - Constant folding in comparisons
 *   - Label coalescing (multiple labels at same PC)
 *
 * Note: After optimization, label map may have dangling references.
 *       The VM handles this gracefully by treating invalid labels as failures.
 */
void PatternBytecode::optimize()
{
	// TODO: Split into multiple pass functions for better organization

	/*-----------------------------------------------------------------------
	  Pass 1: Dead Branch Elimination

	  Detect pattern where a boolean is set to true (non-zero) and then
	  immediately used in BRANCH_FALSE. Since the branch condition is
	  always false, the branch never executes - remove both instructions.
	-----------------------------------------------------------------------*/
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
			if (dstBool && immMint && jumpBool && (dstBool == jumpBool) && (immMint->v != 0))
			{
				// Dead code: branch never taken, remove both instructions
				instrs.erase(instrs.begin() + i, instrs.begin() + i + 2);
				continue; // Check same position again (don't increment)
			}
		}
		++i;
	}

	// Pass 2: Unreachable code elimination (TODO)
	// Would need control flow analysis to identify unreachable blocks
}

/*===========================================================================
 Wolfram Language Interface

 These functions provide the bridge between C++ PatternBytecode objects
 and Wolfram Language code. Each function wraps a C++ method and returns
 a Wolfram Expr that can be used from WL.

 These are registered via initializeEmbedMethods() below to create the
 object-oriented interface in Wolfram Language:

   bc = PatternBytecode[...]
   bc["disassemble"]           -> calls disassemble()
   bc["getInstructionCount"]   -> calls getInstructionCount()
   bc["getBlockCount"]         -> calls getBlockCount()
   etc.
===========================================================================*/
namespace PatternBytecodeInterface
{
	/// Convert bytecode to rich disassembly format
	Expr disassemble(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(bytecode->disassemble());
	}

	/// Get number of boolean registers allocated
	Expr getBoolRegisterCount(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->getBoolRegisterCount()));
	}

	/// Get number of expression registers allocated
	Expr getExprRegisterCount(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->getExprRegisterCount()));
	}

	/// Get total number of instructions
	Expr getInstructionCount(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->getInstructionCount()));
	}

	/// Get number of labels (jump targets)
	Expr getLabelCount(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->getLabelCount()));
	}

	/// Get number of BEGIN_BLOCK instructions
	Expr getBlockCount(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->getBlockCount()));
	}

	/// Get maximum block nesting depth
	Expr getMaxBlockDepth(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->getMaxBlockDepth()));
	}

	/// Get number of jump instructions (JUMP + BRANCH_FALSE)
	Expr getJumpCount(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->getJumpCount()));
	}

	/// Get number of backtrack points (TRY instructions)
	Expr getBacktrackPointCount(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->getBacktrackPointCount()));
	}

	/// Get lexical bindings as Association
	Expr getLexicalBindings(std::shared_ptr<PatternBytecode> bytecode)
	{
		return bytecode->getLexicalBindings();
	}

	/// Get the original pattern that was compiled to this bytecode
	Expr getPattern(std::shared_ptr<PatternBytecode> bytecode)
	{
		return MExpr::toExpr(bytecode->getPattern());
	}

	/// Get total number of instructions in bytecode
	Expr length(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->length()));
	}

	/// Run optimization passes on bytecode (modifies in-place)
	Expr optimize(std::shared_ptr<PatternBytecode> bytecode)
	{
		bytecode->optimize();
		return Expr::ToExpression("Null");
	}

	/// Convert bytecode object to formatted boxes for notebook display
	Expr toBoxes(Expr objExpr, Expr fmt)
	{
		return Expr::construct("DanielS`PatternMatcher`BackEnd`PatternBytecode`Private`toBoxes", objExpr, fmt);
	}

	/// Convert bytecode to compact string format (for tests)
	Expr toString(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(bytecode->toString());
	}
}; // namespace PatternBytecodeInterface

/*===========================================================================
 Method Registration

 Register all PatternBytecode methods with the Wolfram Language embedding
 system. This creates the object-oriented interface where bytecode objects
 can call methods using bc["methodName"] syntax.

 Called once during library initialization.
===========================================================================*/
void PatternBytecode::initializeEmbedMethods(const char* embedName)
{
	// Register each interface function as a callable method
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::disassemble>(embedName, "disassemble");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getBoolRegisterCount>(
		embedName, "getBoolRegisterCount");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getExprRegisterCount>(
		embedName, "getExprRegisterCount");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getInstructionCount>(
		embedName, "getInstructionCount");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getLabelCount>(embedName, "getLabelCount");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getBlockCount>(embedName, "getBlockCount");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getMaxBlockDepth>(embedName,
																								 "getMaxBlockDepth");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getJumpCount>(embedName, "getJumpCount");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getBacktrackPointCount>(
		embedName, "getBacktrackPointCount");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getLexicalBindings>(
		embedName, "getLexicalBindings");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getPattern>(embedName, "getPattern");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::length>(embedName, "length");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::optimize>(embedName, "optimize");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::toBoxes>(embedName, "toBoxes");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::toString>(embedName, "toString");
}
}; // namespace PatternMatcher