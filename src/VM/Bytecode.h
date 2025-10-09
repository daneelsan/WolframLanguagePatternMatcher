#pragma once

#include "VM/Opcode.h"

#include "ClassSupport.h"
#include "Expr.h"

#include <initializer_list>
#include <string>
#include <unordered_map>
#include <vector>

namespace PatternMatcher
{
struct BytecodeInstruction
{
	Opcode opcode;
	std::vector<Operand> ops;
};

class Bytecode
{
private:
	std::vector<BytecodeInstruction> _instrs;

	// metadata
	int exprRegisterCount = 0; // number of registers used for Expr values
	int boolRegisterCount = 0; // number of boolean registers (optional)
	std::unordered_map<std::string, ExprRegIndex> lexicalMap; // pattern variable -> reg

public:
	Bytecode() = default;
	~Bytecode() = default;

	/// @brief Get the instructions of the bytecode.
	const std::vector<BytecodeInstruction>& getInstructions() const { return _instrs; }

	// convenience
	void push_instr(Opcode op, std::initializer_list<Operand> ops_);

	void set_metadata(int exprRegs, int boolRegs, const std::unordered_map<std::string, ExprRegIndex>& lexical)
	{
		exprRegisterCount = exprRegs;
		boolRegisterCount = boolRegs;
		// TODO: move?
		lexicalMap = lexical;
	}

	size_t length() const { return _instrs.size(); }

	/// @brief Converts the bytecode to a string representation.
	/// @return The string representation of the bytecode.
	std::string toString() const;

	/// @brief Compiles a pattern expression into bytecode.
	/// @param expr The pattern expression to compile.
	/// @return A shared pointer to the compiled bytecode.
	static std::shared_ptr<Bytecode> CompilePatternToBytecode(const Expr& expr);

	/// @brief Initializes the embedded methods for the Bytecode class.
	/// @param embedName The name to use for embedding.
	void initializeEmbedMethods(const char* embedName);
};

template <>
inline const char* EmbedName<Bytecode>()
{
	return "PatternMatcherLibrary`VM`PatternBytecode";
}
}; // namespace PatternMatcher
