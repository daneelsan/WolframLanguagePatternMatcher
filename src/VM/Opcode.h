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
	// data movement / registers
	MOVE,/*              A B             R[A] := R[B]                        */
	LOAD_IMM,/*          A imm           R[A] := imm (Expr or literal)       */
	LOAD_INPUT,/*        A               R[A] := input_expr                  */

	// introspection
	GET_HEAD,/*          A B             R[A] := head(R[B])                  */
	GET_PART,/*          A B i           R[A] := part(R[B], i)               */
	TEST_LENGTH,/*       A B i           R[A] := (length(R[B]) == i)         */

	// comparisons / booleans
	SAMEQ,/*             A B C           R[A] := sameQ(R[B], R[C]) (A is bool reg) */
	NOT,/*               A B             R[A] := not R[B]                    */

	// pattern primitives
	BIND_VAR,/*          varName A       Bind lexical var `varName` := R[A]  */
	GET_VAR,/*           A varName       R[A] := bound(varName)              */

	PATTERN_TEST,/*      A predExpr      R[A] := predExpr(R[expr]) -> bool   */

	// control
	JUMP,/*              label           pc = label                          */
	JUMP_IF_FALSE,/*     A label         if !R[A] pc = label                 */
	HALT,/*              <no operands>   stop execution                      */

	// bookkeeping ops (begin/end block)
	BEGIN_BLOCK,/*      label.           signals the beginning of a block    */
	END_BLOCK,/*        label.           signals the end of a block          */

	// Debug / helper
	DEBUG_PRINT,/*       A               print R[A] (for debugging)          */
};
// clang-format on

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
