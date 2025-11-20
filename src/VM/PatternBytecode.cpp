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
/// @brief Convert instruction to string
std::string instructionToString(const PatternBytecode::Instruction& instr)
{
	std::stringstream ss;

	// Opcode name
	const char* name = opcodeName(instr.opcode);
	ss << std::left << std::setw(16) << name;

	// Operands
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

std::string PatternBytecode::toString() const
{
	std::stringstream ss;

	// Build reverse label map (pc -> label)
	std::unordered_map<size_t, Label> pcToLabel;
	for (const auto& [label, pc] : labelMap)
		pcToLabel[pc] = label;

	// Find maximum PC width for alignment
	size_t maxPCWidth = std::to_string(instrs.size() - 1).length();

	// Print instructions (compact format - no indentation, no blank lines)
	for (size_t pc = 0; pc < instrs.size(); ++pc)
	{
		const auto& instr = instrs[pc];

		// Show label if this PC is a target
		auto labelIt = pcToLabel.find(pc);
		if (labelIt != pcToLabel.end())
		{
			ss << "\nL" << labelIt->second << ":\n";
		}

		// PC and instruction
		ss << std::setw(maxPCWidth) << pc << "    ";
		ss << instructionToString(instr);
		ss << "\n";
	}

	// Footer with basic summary
	ss << "\n";
	ss << "----------------------------------------\n";
	ss << "Expr registers: " << exprRegisterCount << ", Bool registers: " << boolRegisterCount << "\n";

	return ss.str();
}

std::string PatternBytecode::disassemble() const
{
	std::stringstream ss;

	// Build reverse label map (pc -> label)
	std::unordered_map<size_t, Label> pcToLabel;
	for (const auto& [label, pc] : labelMap)
		pcToLabel[pc] = label;

	// Analyze structure for indentation
	int currentBlockDepth = 0;
	std::unordered_map<size_t, int> pcToDepth;

	// First pass: calculate indentation depth
	for (size_t pc = 0; pc < instrs.size(); ++pc)
	{
		const auto& instr = instrs[pc];

		// END_BLOCK should be at the same level as its matching BEGIN_BLOCK
		if (instr.opcode == Opcode::END_BLOCK && currentBlockDepth > 0)
			currentBlockDepth--;

		// Assign depth to this instruction
		pcToDepth[pc] = currentBlockDepth;

		// BEGIN_BLOCK: next instruction starts indented
		if (instr.opcode == Opcode::BEGIN_BLOCK)
			currentBlockDepth++;
	}

	// Find maximum PC width for alignment
	size_t maxPCWidth = std::to_string(instrs.size() - 1).length();

	// Statistics tracking
	int blockCount = 0;
	int maxBlockDepth = 0;
	int jumpCount = 0;
	int backtrackPoints = 0;

	// Print instructions with indentation
	for (size_t pc = 0; pc < instrs.size(); ++pc)
	{
		const auto& instr = instrs[pc];
		int depth = pcToDepth[pc];
		maxBlockDepth = std::max(maxBlockDepth, depth);

		// Update statistics
		if (instr.opcode == Opcode::BEGIN_BLOCK)
			blockCount++;
		if (instr.opcode == Opcode::JUMP || instr.opcode == Opcode::BRANCH_FALSE)
			jumpCount++;
		if (instr.opcode == Opcode::TRY)
			backtrackPoints++;

		// Show label if this PC is a target (no blank line before)
		auto labelIt = pcToLabel.find(pc);
		if (labelIt != pcToLabel.end())
		{
			ss << "L" << labelIt->second << ":\n";
		}

		// PC with indentation
		ss << std::setw(maxPCWidth) << pc << "    ";

		// Indentation based on block depth
		for (int i = 0; i < depth; ++i)
			ss << "  ";

		// Instruction
		ss << instructionToString(instr);

		// Inline annotations for jumps
		if (instr.opcode == Opcode::JUMP || instr.opcode == Opcode::BRANCH_FALSE)
		{
			// Find label operand
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

	// Footer with enhanced statistics
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

std::optional<size_t> PatternBytecode::resolveLabel(Label L) const
{
	auto it = labelMap.find(L);
	if (it != labelMap.end())
		return it->second;
	throw std::nullopt;
}

void PatternBytecode::optimize()
{
	// TODO: split in multiple passes for different optimizations

	// Pass 1: Remove BRANCH_FALSE with always-true conditions
	for (size_t i = 0; i + 1 < instrs.size();)
	{
		// Pattern: LOAD_IMM %b, 1 followed by BRANCH_FALSE %b, L
		if (instrs[i].opcode == Opcode::LOAD_IMM && instrs[i + 1].opcode == Opcode::BRANCH_FALSE)
		{
			auto dstBool = std::get_if<BoolRegOp>(&instrs[i].ops[0]);
			auto immMint = std::get_if<ImmMint>(&instrs[i].ops[1]);
			auto jumpBool = std::get_if<BoolRegOp>(&instrs[i + 1].ops[0]);

			if (dstBool && immMint && jumpBool && (dstBool == jumpBool) && (immMint->v != 0))
			{
				// This is a dead jump - remove both instructions
				instrs.erase(instrs.begin() + i, instrs.begin() + i + 2);
				continue; // Don't increment i
			}
		}
		++i;
	}

	// Pass 2: Remove unreachable code after unconditional jumps
	// (More complex - would need to track which labels are targets)
}

namespace PatternBytecodeInterface
{
	Expr disassemble(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(bytecode->disassemble());
	}
	Expr getBoolRegisterCount(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->getBoolRegisterCount()));
	}
	Expr getExprRegisterCount(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->getExprRegisterCount()));
	}
	Expr getPattern(std::shared_ptr<PatternBytecode> bytecode)
	{
		return MExpr::toExpr(bytecode->getPattern());
	}
	Expr length(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->length()));
	}
	Expr optimize(std::shared_ptr<PatternBytecode> bytecode)
	{
		bytecode->optimize();
		return Expr::ToExpression("Null");
	}
	Expr toBoxes(Expr objExpr, Expr fmt)
	{
		return Expr::construct("DanielS`PatternMatcher`BackEnd`PatternBytecode`Private`toBoxes", objExpr, fmt);
	}
	Expr toString(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(bytecode->toString());
	}
}; // namespace PatternBytecodeInterface

void PatternBytecode::initializeEmbedMethods(const char* embedName)
{
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::disassemble>(embedName, "disassemble");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getBoolRegisterCount>(
		embedName, "getBoolRegisterCount");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getExprRegisterCount>(
		embedName, "getExprRegisterCount");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getPattern>(embedName, "getPattern");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::length>(embedName, "length");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::optimize>(embedName, "optimize");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::toBoxes>(embedName, "toBoxes");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::toString>(embedName, "toString");
}
}; // namespace PatternMatcher