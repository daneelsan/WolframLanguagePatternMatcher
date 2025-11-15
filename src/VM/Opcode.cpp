#include "VM/Opcode.h"

#include <string>
#include <variant>

namespace PatternMatcher
{
const char* opcodeName(Opcode op)
{
	switch (op)
	{
		case Opcode::BEGIN_BLOCK:
			return "BEGIN_BLOCK";
		case Opcode::BIND_VAR:
			return "BIND_VAR";
		case Opcode::BRANCH_FALSE:
			return "BRANCH_FALSE";
		case Opcode::CUT:
			return "CUT";
		case Opcode::DEBUG_PRINT:
			return "DEBUG_PRINT";
		case Opcode::END_BLOCK:
			return "END_BLOCK";
		case Opcode::EXPORT_BINDINGS:
			return "EXPORT_BINDINGS";
		case Opcode::FAIL:
			return "FAIL";
		case Opcode::GET_PART:
			return "GET_PART";
		case Opcode::HALT:
			return "HALT";
		case Opcode::JUMP:
			return "JUMP";
		case Opcode::LOAD_IMM:
			return "LOAD_IMM";
		case Opcode::MATCH_HEAD:
			return "MATCH_HEAD";
		case Opcode::MATCH_LENGTH:
			return "MATCH_LENGTH";
		case Opcode::MATCH_LITERAL:
			return "MATCH_LITERAL";
		case Opcode::MOVE:
			return "MOVE";
		case Opcode::RETRY:
			return "RETRY";
		case Opcode::SAMEQ:
			return "SAMEQ";
		case Opcode::TRUST:
			return "TRUST";
		case Opcode::TRY:
			return "TRY";
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
		return "Label[" + std::to_string(std::get<LabelOp>(op).v) + "]";
	}
	if (std::holds_alternative<Ident>(op))
	{
		return "Symbol[\"" + std::get<Ident>(op) + "\"]";
	}
	if (std::holds_alternative<ImmExpr>(op))
	{
		auto expr = std::get<ImmExpr>(op);
		return "Expr[" + expr.toInputFormString() + "]";
	}
	if (std::holds_alternative<ImmMint>(op))
	{
		auto n = std::get<ImmMint>(op);
		return std::to_string(n.v);
	}
	return "<none>";
}
}; // namespace PatternMatcher