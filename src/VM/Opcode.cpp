#include "VM/Opcode.h"

#include <string>
#include <variant>

namespace PatternMatcher
{
const char* opcodeName(Opcode op)
{
	switch (op)
	{
		case Opcode::MOVE:
			return "MOVE";
		case Opcode::LOAD_IMM:
			return "LOAD_IMM";
		case Opcode::LOAD_INPUT:
			return "LOAD_INPUT";
		case Opcode::GET_HEAD:
			return "GET_HEAD";
		case Opcode::GET_PART:
			return "GET_PART";
		case Opcode::TEST_LENGTH:
			return "TEST_LENGTH";
		case Opcode::SAMEQ:
			return "SAMEQ";
		case Opcode::NOT:
			return "NOT";
		case Opcode::BIND_VAR:
			return "BIND_VAR";
		case Opcode::GET_VAR:
			return "GET_VAR";
		case Opcode::PATTERN_TEST:
			return "PATTERN_TEST";
		case Opcode::JUMP:
			return "JUMP";
		case Opcode::JUMP_IF_FALSE:
			return "JUMP_IF_FALSE";
		case Opcode::HALT:
			return "HALT";
		case Opcode::BEGIN_BLOCK:
			return "BEGIN_BLOCK";
		case Opcode::END_BLOCK:
			return "END_BLOCK";
		case Opcode::DEBUG_PRINT:
			return "DEBUG_PRINT";
		default:
			return "OP_UNKNOWN";
	}
}

std::string operandToString(const Operand& op)
{
	if (std::holds_alternative<ExprRegOp>(op))
	{
		return "%e" + std::to_string(std::get<ExprRegOp>(op).v);
	}
	if (std::holds_alternative<BoolRegOp>(op))
	{
		return "%b" + std::to_string(std::get<BoolRegOp>(op).v);
	}
	if (std::holds_alternative<LabelOp>(op))
	{
		return "L" + std::to_string(std::get<LabelOp>(op).v);
	}
	if (std::holds_alternative<Ident>(op))
	{
		return "id:`" + std::get<Ident>(op) + "`";
	}
	if (std::holds_alternative<ImmExpr>(op))
	{
		auto expr = std::get<ImmExpr>(op);
		return "Expr(\"" + expr.toInputFormString() + "\")";
	}
	if (std::holds_alternative<ImmMint>(op))
	{
		auto n = std::get<ImmMint>(op);
		return std::to_string(n.v);
	}
	return "<none>";
}
}; // namespace PatternMatcher