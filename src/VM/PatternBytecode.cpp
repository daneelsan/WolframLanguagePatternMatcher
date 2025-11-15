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
	ss << std::left << std::setw(16) << opcodeName(instr.opcode);

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

	// Print instructions
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

	// Footer with summary
	ss << "\n";
	ss << "----------------------------------------\n";
	ss << "Expr registers: " << exprRegisterCount << ", Bool registers: " << boolRegisterCount << "\n";

	if (!lexicalMap.empty())
	{
		ss << "Lexical bindings:\n";
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