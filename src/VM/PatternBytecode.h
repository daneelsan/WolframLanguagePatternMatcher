#pragma once

#include "VM/Opcode.h"

#include "ClassSupport.h"
#include "Expr.h"

#include "AST/MExpr.h"

#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace PatternMatcher
{
class PatternBytecode
{

public:
	struct Instruction
	{
		Opcode opcode;
		std::vector<Operand> ops;
	};

	PatternBytecode() = default;
	~PatternBytecode() = default;

	/// @brief Get the instructions of the bytecode.
	const std::vector<Instruction>& getInstructions() const { return instrs; }

	/// @brief Get the length of the bytecode.
	/// @return The length of the bytecode in bytes.
	size_t length() const { return instrs.size(); }

	// Simple O(1) queries

	/// @brief Get the number of instructions in the bytecode.
	int getInstructionCount() const { return instrs.size(); }

	/// @brief Get the number of labels in the bytecode.
	int getLabelCount() const { return labelMap.size(); }

	/// @brief Get the number of expression registers used.
	int getExprRegisterCount() const { return exprRegisterCount; }

	/// @brief Get the number of boolean registers used.
	int getBoolRegisterCount() const { return boolRegisterCount; }

	/// @brief Get the original pattern expression.
	std::shared_ptr<MExpr> getPattern() const { return pattern; }

	/// @brief Count total number of BEGIN_BLOCK instructions
	int getBlockCount() const;

	/// @brief Get maximum block nesting depth
	int getMaxBlockDepth() const;

	/// @brief Count total number of jump instructions (JUMP + BRANCH_FALSE)
	int getJumpCount() const;

	/// @brief Count number of backtracking choice points (TRY instructions)
	int getBacktrackPointCount() const;

	/// @brief Get the lexical bindings as an Association.
	Expr getLexicalBindings() const;

	/// @brief Add an instruction to the bytecode.
	/// @param op The opcode of the instruction.
	/// @param ops_ The operands of the instruction.
	void push_instr(Opcode op, std::initializer_list<Operand> ops_)
	{
		instrs.push_back(Instruction { op, std::vector<Operand>(ops_) });
	}

	/// @brief Add a label to the bytecode.
	/// @param L The label to add.
	void addLabel(Label L) { labelMap[L] = instrs.size(); }

	std::optional<size_t> resolveLabel(Label L) const;

	void set_metadata(std::shared_ptr<MExpr> pattern, int exprRegs, int boolRegs,
					  const std::unordered_map<std::string, ExprRegIndex>& lexical)
	{
		this->pattern = pattern;
		this->exprRegisterCount = exprRegs;
		this->boolRegisterCount = boolRegs;
		// TODO: move?
		this->lexicalMap = lexical;
	}

	/// @brief Converts the bytecode to a string representation (compact format for tests).
	/// @return The string representation of the bytecode.
	std::string toString() const;

	/// @brief Converts the bytecode to a formatted disassembly (user-friendly with indentation).
	/// @return The formatted disassembly of the bytecode.
	std::string disassemble() const;

	/// @brief Optimize the bytecode (e.g., remove no-op instructions).
	void optimize();

	/// @brief Initializes the embedded methods for the Bytecode class.
	/// @param embedName The name to use for embedding.
	void initializeEmbedMethods(const char* embedName);

private:
	std::shared_ptr<MExpr> pattern; // original pattern expression (for reference/debugging)
	std::vector<Instruction> instrs;

	// metadata
	int exprRegisterCount = 0; // number of registers used for Expr values
	int boolRegisterCount = 0; // number of boolean registers (optional)
	std::unordered_map<std::string, ExprRegIndex> lexicalMap; // pattern variable -> reg
	std::unordered_map<Label, size_t> labelMap;
};

template <>
inline const char* EmbedName<PatternBytecode>()
{
	return "PatternMatcherLibrary`VM`PatternBytecode";
}
}; // namespace PatternMatcher
