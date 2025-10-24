#include "VM/Opcode.h"

#include <string>
#include <variant>

namespace PatternMatcher
{
const char* opcodeName(Opcode op)
{
	switch (op)
	{
		// Register operations
		case Opcode::LOAD_IMM:
			return "LOAD_IMM";
		case Opcode::LOAD_INPUT:
			return "LOAD_INPUT";
		case Opcode::MOVE:
			return "MOVE";

		// MatchOp
		// MATCH_XXXX EXPR_REG EXPR_IMM LBL
		case Opcode::MATCH_HEAD:
			return "MATCH_HEAD";
		case Opcode::MATCH_LITERAL:
			return "MATCH_LITERAL";
		case Opcode::MATCH_LENGTH:
			return "MATCH_LENGTH";

		// Introspection
		case Opcode::GET_HEAD:
			return "GET_HEAD";
		case Opcode::GET_PART:
			return "GET_PART";

		// Tests
		case Opcode::TEST_LENGTH:
			return "TEST_LENGTH";
		case Opcode::SAMEQ:
			return "SAMEQ";

		case Opcode::NOT:
			return "NOT";

		// Pattern Primitives
		case Opcode::BIND_VAR:
			return "BIND_VAR";
		case Opcode::GET_VAR:
			return "GET_VAR";
		case Opcode::PATTERN_TEST:
			return "PATTERN_TEST";

		// Control Flow
		case Opcode::JUMP:
			return "JUMP";
		case Opcode::JUMP_IF_FALSE:
			return "JUMP_IF_FALSE";
		case Opcode::HALT:
			return "HALT";

		// Scope Management
		case Opcode::BEGIN_BLOCK:
			return "BEGIN_BLOCK";
		case Opcode::END_BLOCK:
			return "END_BLOCK";

		// Backtracking operations
		case Opcode::TRY:
			return "TRY";
		case Opcode::RETRY:
			return "RETRY";
		case Opcode::TRUST:
			return "TRUST";
		case Opcode::CHOICE_POINT:
			return "CHOICE_POINT";
		case Opcode::CUT:
			return "CUT";
		case Opcode::FAIL:
			return "FAIL";
		case Opcode::TRAIL_BIND:
			return "TRAIL_BIND";
		case Opcode::SAVE_STATE:
			return "SAVE_STATE";
		case Opcode::RESTORE_STATE:
			return "RESTORE_STATE";

		// Debug / Helper
		case Opcode::NOP:
			return "NOP";
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