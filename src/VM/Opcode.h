#pragma once

#include "Expr.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace PatternMatcher
{
class Expr; // forward declaration

//=============================================================================
// Opcode: Intermediate representation for pattern matching bytecode
//=============================================================================
// clang-format off
enum class Opcode {
/*-----------------------------------------------------------------------------
        name             args            description
-----------------------------------------------------------------------------*/
	// ===== Data Movement / Registers =====
	MOVE,            /*  A B             R[A] := R[B]                        */
	LOAD_IMM,        /*  A imm           R[A] := imm (Expr or literal)       */
	LOAD_INPUT,      /*  A               R[A] := input_expr                  */

	// ===== Introspection =====
	GET_HEAD,        /*  A B             R[A] := head(R[B])                  */
	GET_PART,        /*  A B i           R[A] := part(R[B], i)               */
	TEST_LENGTH,     /*  A B i           R[A] := (length(R[B]) == i)         */

	// ===== Combined Match Operations (Optimized) =====
	MATCH_HEAD,      /*  A imm label     if (head(R[A]) != imm) pc = label   */
	MATCH_LENGTH,    /*  A len label     if (length(R[A]) != len) pc = label */
	MATCH_LITERAL,   /*  A imm label     if (R[A] != imm) pc = label         */

	// ===== Comparisons / Booleans =====
	SAMEQ,           /*  A B C           R[A] := sameQ(R[B], R[C]) (bool)    */
	NOT,             /*  A B             R[A] := not R[B]                    */

	// ===== Pattern Primitives =====
	BIND_VAR,        /*  varName A       Bind lexical var `varName` := R[A]  */
	GET_VAR,         /*  A varName       R[A] := bound(varName)              */
	PATTERN_TEST,    /*  A predExpr      R[A] := predExpr(R[expr]) -> bool   */

	// ===== Control Flow =====
	JUMP,            /*  label           pc = label                          */
	JUMP_IF_FALSE,   /*  A label         if !R[A] pc = label                 */
	HALT,            /*  <no operands>   stop execution                      */

	// ===== Scope Management =====
	BEGIN_BLOCK,     /*  label           signals the beginning of a block              */
	END_BLOCK,       /*  label           signals the end of a block                    */

	// ===== Backtracking Support =====
	TRY,             /*  label           Create choice point, try first alternative    */
	RETRY,           /*  label           Backtrack to choice point, try next alt       */
	TRUST,           /*  label           Last alternative, remove choice point         */
	CHOICE_POINT,    /*  label           Explicit choice point creation                */
	CUT,             /*  <no operands>   Remove choice points up to current frame      */
	FAIL,            /*  <no operands>   Force backtracking                            */

	// ===== Trail Management =====
	TRAIL_BIND,      /*  varName A       Bind var with trail entry for backtracking   */
	SAVE_STATE,      /*  <no operands>   Explicitly save registers to choice point    */
	RESTORE_STATE,   /*  <no operands>   Restore registers from choice point          */

	// ===== Debug / Helper =====
	DEBUG_PRINT,     /*  A               print R[A] (for debugging)                    */
	NOP,             /*  <no operands>   no operation (for alignment/debug)            */
};
// clang-format on

/// @brief Opcode category for grouping and analysis
enum class OpcodeCategory
{
	DataMovement,
	Introspection,
	MatchOp,
	Comparison,
	Pattern,
	ControlFlow,
	ScopeManagement,
	Backtracking,
	Debug
};

/// @brief Get the category of an opcode
inline OpcodeCategory getOpcodeCategory(Opcode op)
{
	switch (op)
	{
		case Opcode::MOVE:
		case Opcode::LOAD_IMM:
		case Opcode::LOAD_INPUT:
			return OpcodeCategory::DataMovement;

		case Opcode::GET_HEAD:
		case Opcode::GET_PART:
		case Opcode::TEST_LENGTH:
			return OpcodeCategory::Introspection;

		case Opcode::MATCH_HEAD:
		case Opcode::MATCH_LENGTH:
		case Opcode::MATCH_LITERAL:
			return OpcodeCategory::MatchOp;

		case Opcode::SAMEQ:
		case Opcode::NOT:
			return OpcodeCategory::Comparison;

		case Opcode::BIND_VAR:
		case Opcode::GET_VAR:
		case Opcode::PATTERN_TEST:
			return OpcodeCategory::Pattern;

		case Opcode::JUMP:
		case Opcode::JUMP_IF_FALSE:
		case Opcode::HALT:
			return OpcodeCategory::ControlFlow;

		case Opcode::BEGIN_BLOCK:
		case Opcode::END_BLOCK:
			return OpcodeCategory::ScopeManagement;

		case Opcode::TRY:
		case Opcode::RETRY:
		case Opcode::TRUST:
		case Opcode::CHOICE_POINT:
		case Opcode::CUT:
		case Opcode::FAIL:
		case Opcode::TRAIL_BIND:
		case Opcode::SAVE_STATE:
		case Opcode::RESTORE_STATE:
			return OpcodeCategory::Backtracking;

		case Opcode::DEBUG_PRINT:
		case Opcode::NOP:
			return OpcodeCategory::Debug;

		default:
			return OpcodeCategory::Debug;
	}
}

/// @brief Check if opcode is a control flow instruction
inline bool isControlFlow(Opcode op)
{
	return getOpcodeCategory(op) == OpcodeCategory::ControlFlow;
}

/// @brief Check if opcode is a branch instruction
inline bool isBranch(Opcode op)
{
	return op == Opcode::JUMP || op == Opcode::JUMP_IF_FALSE || op == Opcode::MATCH_HEAD || op == Opcode::MATCH_LENGTH
		|| op == Opcode::MATCH_LITERAL;
}

/// @brief Get human-readable description of opcode
inline const char* getOpcodeDescription(Opcode op)
{
	switch (op)
	{
		case Opcode::MOVE:
			return "Copy value between registers";
		case Opcode::LOAD_IMM:
			return "Load immediate value";
		case Opcode::LOAD_INPUT:
			return "Load input expression";
		case Opcode::GET_HEAD:
			return "Extract head of expression";
		case Opcode::GET_PART:
			return "Extract part of expression";
		case Opcode::TEST_LENGTH:
			return "Test expression length";
		case Opcode::MATCH_HEAD:
			return "Match head and branch";
		case Opcode::MATCH_LENGTH:
			return "Match length and branch";
		case Opcode::MATCH_LITERAL:
			return "Match literal and branch";
		case Opcode::SAMEQ:
			return "Test structural equality";
		case Opcode::NOT:
			return "Boolean negation";
		case Opcode::BIND_VAR:
			return "Bind pattern variable";
		case Opcode::GET_VAR:
			return "Retrieve bound variable";
		case Opcode::PATTERN_TEST:
			return "Apply pattern test";
		case Opcode::JUMP:
			return "Unconditional jump";
		case Opcode::JUMP_IF_FALSE:
			return "Conditional jump";
		case Opcode::HALT:
			return "Stop execution";
		case Opcode::BEGIN_BLOCK:
			return "Begin lexical scope";
		case Opcode::END_BLOCK:
			return "End lexical scope";
		case Opcode::DEBUG_PRINT:
			return "Print debug information";
		case Opcode::NOP:
			return "No operation";
		default:
			return "Unknown opcode";
	}
}

//=============================================================================
// Register and operand types
//=============================================================================

using ExprRegIndex = size_t; // register index for Expr registers
using BoolRegIndex = size_t; // register index for boolean registers
using Label = size_t; // label (instruction index) for jumps
using Ident = std::string; // identifier (e.g., variable name)
using ImmExpr = Expr; // immediate expression (for LOAD_IMM, etc.)

// --- Operand wrapper types (distinct C++ types so std::variant can distinguish them) ---
struct ExprRegOp
{
	ExprRegIndex v;

	bool operator==(const ExprRegOp& other) const { return v == other.v; }
	bool operator!=(const ExprRegOp& other) const { return v != other.v; }
};

struct BoolRegOp
{
	BoolRegIndex v;

	bool operator==(const BoolRegOp& other) const { return v == other.v; }
	bool operator!=(const BoolRegOp& other) const { return v != other.v; }
};

struct LabelOp
{
	Label v;

	bool operator==(const LabelOp& other) const { return v == other.v; }
	bool operator!=(const LabelOp& other) const { return v != other.v; }
};

struct ImmMint
{
	mint v;

	bool operator==(const ImmMint& other) const { return v == other.v; }
	bool operator!=(const ImmMint& other) const { return v != other.v; }
};

// Operand is a variant which can be a register wrapper, bool wrapper, a label wrapper,
// an identifier (string), an Expr immediate, a mint literal, or a bool literal.
using Operand = std::variant<std::monostate, ExprRegOp, BoolRegOp, LabelOp, Ident, ImmExpr, ImmMint>;

// helpers that construct operands easily from ints/Expr/string
inline Operand OpExprReg(ExprRegIndex r)
{
	return Operand { ExprRegOp { r } };
}
inline Operand OpBoolReg(BoolRegIndex b)
{
	return Operand { BoolRegOp { b } };
}
inline Operand OpLabel(Label L)
{
	return Operand { LabelOp { L } };
}
inline Operand OpIdent(const std::string& s)
{
	return Operand { s };
}
inline Operand OpImm(const Expr& e)
{
	return Operand { e };
}
inline Operand OpImm(mint v)
{
	return Operand { ImmMint { v } };
}

//=============================================================================
// Utility functions
//=============================================================================

/// @brief Get the name of an opcode as a string.
const char* opcodeName(Opcode op);

/// @brief Convert an operand to a string representation.
std::string operandToString(const Operand& op);

// Optional: Helper to check if an operand is of a specific type
template <typename T>
bool isOperandType(const Operand& op)
{
	return std::holds_alternative<T>(op);
}

// Optional: Safe getter with optional return
template <typename T>
std::optional<T> getOperandAs(const Operand& op)
{
	if (auto* ptr = std::get_if<T>(&op))
		return *ptr;
	return std::nullopt;
}
}; // namespace PatternMatcher
