#pragma once

#include "VM/Opcode.h"

#include "ClassSupport.h"
#include "Expr.h"

#include "AST/MExpr.h"

#include <initializer_list>
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
	const std::vector<Instruction>& getInstructions() const { return _instrs; }

	/// @brief Get the length of the bytecode.
	/// @return The length of the bytecode in bytes.
	size_t length() const { return _instrs.size(); }

	int getExprRegisterCount() const { return exprRegisterCount; }
	int getBoolRegisterCount() const { return boolRegisterCount; }
	std::shared_ptr<MExpr> getPattern() const { return pattern; }

	/// @brief Add an instruction to the bytecode.
	/// @param op The opcode of the instruction.
	/// @param ops_ The operands of the instruction.
	void push_instr(Opcode op, std::initializer_list<Operand> ops_)
	{
		_instrs.push_back(Instruction { op, std::vector<Operand>(ops_) });
	}

	/// @brief Add a label to the bytecode.
	/// @param L The label to add.
	void addLabel(Label L) { labelMap[L] = _instrs.size(); }

	void set_metadata(std::shared_ptr<MExpr> pattern, int exprRegs, int boolRegs,
					  const std::unordered_map<std::string, ExprRegIndex>& lexical)
	{
		this->pattern = pattern;
		this->exprRegisterCount = exprRegs;
		this->boolRegisterCount = boolRegs;
		// TODO: move?
		this->lexicalMap = lexical;
	}

	/// @brief Converts the bytecode to a string representation.
	/// @return The string representation of the bytecode.
	std::string toString() const;

	/// @brief Compiles a pattern expression into bytecode.
	/// @param expr The pattern expression to compile.
	/// @return A shared pointer to the compiled bytecode.
	static std::shared_ptr<PatternBytecode> CompilePatternToBytecode(const Expr& expr);

	/// @brief Initializes the embedded methods for the Bytecode class.
	/// @param embedName The name to use for embedding.
	void initializeEmbedMethods(const char* embedName);

private:
	std::shared_ptr<MExpr> pattern; // original pattern expression (for reference/debugging)
	std::vector<Instruction> _instrs;

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
