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

/*===========================================================================
Opcode: Bytecode Instruction Set for Pattern Matching VM

This file defines the instruction set for the pattern matching virtual machine.
The VM uses a register-based architecture with:
- Expression registers (%e0, %e1, %e2, ...)
- Boolean registers (%b0, %b1, %b2, ...)
- Label-based control flow (like assembly)
- Backtracking support via choice points (like Prolog WAM)

Design Philosophy:
- Keep the instruction set minimal and orthogonal
- Each instruction does one thing well
- Match operations combine testing and branching for efficiency
- Backtracking instructions manage choice points explicitly

Register Conventions:
- %e0: Always holds the current expression being matched
- %b0: Always holds the final match result (true/false)
- Other registers are allocated as needed during compilation
===========================================================================*/

// clang-format off
enum class Opcode {
/*-----------------------------------------------------------------------------
Opcode Instruction Reference

Format: OPCODE   #ops: operands → description [side effects]

All MATCH_* instructions jump to a failure label if the test fails.
All backtracking instructions manipulate the choice point stack.
-----------------------------------------------------------------------------*/

    //=========================================================================
    // DATA MOVEMENT (2 opcodes)
    // Copy and load data between registers
    //=========================================================================
    
    MOVE,            /*  2: dest src           → dest := src
                           Copy value from src register to dest register.
                           Used to preserve values across operations. */
    
    LOAD_IMM,        /*  2: dest imm           → dest := imm
                           Load an immediate value (Expr or literal) into dest.
                           The immediate is embedded in the bytecode. */

    //=========================================================================
    // EXPRESSION INTROSPECTION (1 opcode)
    // Extract parts of structured expressions
    //=========================================================================
    
    GET_PART,        /*  3: dest src index     → dest := part(src, index)
                           Extract the index-th part of src expression.
                           Example: part(f[a,b,c], 2) = b
                           For index=0, returns the head. */

    //=========================================================================
    // PATTERN MATCHING (3 opcodes)
    // Test expression properties and branch on failure
    // These are "fused" operations: test + conditional jump
    //=========================================================================
    
    MATCH_HEAD,      /*  3: reg head fail      → jump fail if head(reg) != head
                           Test if the head of reg matches expected head.
                           Example: MATCH_HEAD %e0, Integer, L_fail
                             jumps to L_fail if head(%e0) != Integer
                           Used for: _Integer, _Real, f[...] patterns */
    
    MATCH_LENGTH,    /*  3: reg len fail       → jump fail if length(reg) != len
                           Test if the argument count matches expected length.
                           Example: MATCH_LENGTH %e0, 2, L_fail
                             jumps to L_fail if the expression doesn't have 2 args
                           Used for: f[x_, y_] (requires exactly 2 arguments) */
    
    MATCH_LITERAL,   /*  3: reg val fail       → jump fail if reg != val
                           Test if reg matches a literal value exactly.
                           Uses structural equality (SameQ semantics).
                           Example: MATCH_LITERAL %e0, 5, L_fail
                             jumps if %e0 is not exactly 5
                           Used for: literal patterns like 5, "hello", Pi */

    //=========================================================================
    // COMPARISON (1 opcode)
    // Compute equality between expressions
    //=========================================================================
    
    SAMEQ,           /*  3: dest lhs rhs       → dest := (lhs === rhs)
                           Structural equality test (like Mathematica's SameQ).
                           Returns boolean result in dest register.
                           Used for: repeated variable checks (f[x_, x_])
                           Example: SAMEQ %b1, %e3, %e0  tests if %e3 equals %e0 */

    //=========================================================================
    // VARIABLE BINDING (1 opcode)
    // Bind pattern variables to values
    //=========================================================================
    
    BIND_VAR,        /*  2: name reg           → bind(name, reg) [trails]
                           Bind a pattern variable to a value at runtime.
                           The binding is stored in the current frame.
                           Side effect: Creates trail entry for backtracking.
                           Example: BIND_VAR "Global`x", %e3
                             binds the variable x to the value in %e3
                           Used for: x_, x_Integer (after successful match) */

    //=========================================================================
    // CONTROL FLOW (3 opcodes)
    // Jump and halt instructions
    //=========================================================================
    
    JUMP,            /*  1: label              → pc := label
                           Unconditional jump to label.
                           Labels are resolved to instruction indices.
                           Example: JUMP L_success */
    
    BRANCH_FALSE,    /*  2: cond label         → jump label if cond is false
                           Conditional jump based on boolean register.
                           Example: BRANCH_FALSE %b1, L_fail
                             jumps to L_fail if %b1 contains false
                           Used for: repeated variable failure (f[x_, x_]) */
    
    HALT,            /*  0:                    → stop execution
                           Stops the VM and returns control to caller.
                           The result is in %b0 (true/false).
                           Example: Used at end of success/fail blocks */

    //=========================================================================
    // SCOPE MANAGEMENT (3 opcodes)
    // Manage lexical scopes and variable bindings
    //=========================================================================
    
    BEGIN_BLOCK,     /*  1: label              → create frame [pushes]
                           Create a new lexical scope (frame).
                           Variable bindings within this block are scoped.
                           Side effect: Pushes a new frame onto the frame stack.
                           Example: BEGIN_BLOCK L0  starts the entry block
                           Used for: Entry block, nested pattern scopes */
    
    END_BLOCK,       /*  1: label              → pop frame [pops]
                           Exit a lexical scope and pop its frame.
                           Side effect: Pops frame from stack.
                           Bindings in the frame may be merged to parent.
                           Example: END_BLOCK L0  closes the entry block
                           Must be balanced with BEGIN_BLOCK. */
    
    EXPORT_BINDINGS, /*  0:                    → copy bindings to result
                           Export all variable bindings from current frame
                           to the result that will be returned to the caller.
                           Example: EXPORT_BINDINGS  before HALT in success block
                           Used for: Capturing final bindings like {x→5, y→10} */

    //=========================================================================
    // BACKTRACKING (5 opcodes)
    // Manage choice points for alternatives (p1 | p2 | p3)
    // Implements non-deterministic pattern matching like Prolog WAM
    //=========================================================================
    
    TRY,             /*  1: label              → create choice point → label [saves state]
                           Create a choice point for the first alternative.
                           Side effects:
                             - Saves: registers, frame depth, trail position
                             - Pushes choice point onto choice stack
                             - Records label as "next alternative" to try on FAIL
                           Example: TRY L_alt2  creates choice point for alternative 2
                           Used for: First alternative in (p1 | p2 | p3)
                           On backtrack: Restores state and jumps to label */
    
    RETRY,           /*  1: label              → update choice point → label
                           Update the existing choice point to point to next alternative.
                           Side effect: Modifies top choice point's "next alternative".
                           Example: RETRY L_alt3  updates to try alternative 3 on FAIL
                           Used for: Middle alternatives in (p1 | p2 | p3)
                           Does NOT create or remove choice points. */
    
    TRUST,           /*  0:                    → remove choice point [pops]
                           Remove the top choice point (last alternative).
                           Side effect: Pops from choice point stack.
                           Signals: "No more alternatives to try."
                           Example: TRUST  before last alternative
                           Used for: Last alternative in (p1 | p2 | ... | pN)
                           If this alternative fails, the whole pattern fails. */
    
    CUT,             /*  0:                    → remove all choice points [prunes]
                           Remove all choice points up to current frame boundary.
                           Side effect: Prunes the choice stack.
                           Commits to the current choice (like Prolog's !).
                           Example: CUT  after a successful alternative
                           Used for: Optimization (prevent unnecessary backtracking) */
    
    FAIL,            /*  0:                    → trigger backtracking [restores state]
                           Force backtracking to the most recent choice point.
                           Side effects:
                             - Restores: registers, frames, trail (from choice point)
                             - Sets pc to choice point's "next alternative" label
                             - Does NOT pop the choice point (RETRY/TRUST do that)
                           Example: FAIL  when an alternative fails
                           Used for: Triggering backtrack in alternatives
                           If no choice points: Halts with failure. */

    //=========================================================================
    // DEBUG (1 opcode)
    // Debugging and diagnostics
    //=========================================================================
    
    DEBUG_PRINT,     /*  1: reg                → print reg to debug output
                           Print the value of a register for debugging.
                           Has no effect on program semantics.
                           Example: DEBUG_PRINT %e0  prints current expression
                           Used for: Tracing execution, debugging patterns */
};
// clang-format on

//=============================================================================
// Opcode Metadata and Categorization
//=============================================================================

/// @brief Opcode category for grouping and analysis
/// Used for bytecode optimization, analysis, and documentation
enum class OpcodeCategory
{
	DataMovement, // MOVE, LOAD_IMM
	Introspection, // GET_PART
	ConditionalMatch, // MATCH_HEAD, MATCH_LENGTH, MATCH_LITERAL (test + branch)
	Comparison, // SAMEQ
	Binding, // BIND_VAR
	ControlFlow, // JUMP, BRANCH_FALSE, HALT
	ScopeManagement, // BEGIN_BLOCK, END_BLOCK, EXPORT_BINDINGS
	Backtracking, // TRY, RETRY, TRUST, CUT, FAIL
	Debug // DEBUG_PRINT
};

/// @brief Get the category of an opcode
/// @param op The opcode to categorize
/// @return The category enum
inline OpcodeCategory getOpcodeCategory(Opcode op)
{
	switch (op)
	{
		// Data movement
		case Opcode::MOVE:
		case Opcode::LOAD_IMM:
			return OpcodeCategory::DataMovement;

		// Introspection
		case Opcode::GET_PART:
			return OpcodeCategory::Introspection;

		// Conditional matching (fused test + branch)
		case Opcode::MATCH_HEAD:
		case Opcode::MATCH_LENGTH:
		case Opcode::MATCH_LITERAL:
			return OpcodeCategory::ConditionalMatch;

		// Comparisons
		case Opcode::SAMEQ:
			return OpcodeCategory::Comparison;

		// Variable binding
		case Opcode::BIND_VAR:
			return OpcodeCategory::Binding;

		// Control flow
		case Opcode::JUMP:
		case Opcode::BRANCH_FALSE:
		case Opcode::HALT:
			return OpcodeCategory::ControlFlow;

		// Scope management
		case Opcode::BEGIN_BLOCK:
		case Opcode::END_BLOCK:
		case Opcode::EXPORT_BINDINGS:
			return OpcodeCategory::ScopeManagement;

		// Backtracking
		case Opcode::TRY:
		case Opcode::RETRY:
		case Opcode::TRUST:
		case Opcode::CUT:
		case Opcode::FAIL:
			return OpcodeCategory::Backtracking;

		// Debug
		case Opcode::DEBUG_PRINT:
			return OpcodeCategory::Debug;

		default:
			return OpcodeCategory::Debug;
	}
}

/// @brief Check if opcode is a control flow instruction
/// @param op The opcode to check
/// @return true if the opcode transfers control (jump, halt, etc.)
inline bool isControlFlow(Opcode op)
{
	return getOpcodeCategory(op) == OpcodeCategory::ControlFlow;
}

/// @brief Check if opcode is a branch instruction (may change pc)
/// @param op The opcode to check
/// @return true if the opcode can jump to a different label
///
/// Note: This includes both unconditional jumps (JUMP) and conditional
/// branches (BRANCH_FALSE, MATCH_*, FAIL). Used for control flow analysis.
inline bool isBranch(Opcode op)
{
	return op == Opcode::JUMP || op == Opcode::BRANCH_FALSE || op == Opcode::MATCH_HEAD || op == Opcode::MATCH_LENGTH
		|| op == Opcode::MATCH_LITERAL || op == Opcode::FAIL;
}

/// @brief Check if opcode has side effects beyond register writes
/// @param op The opcode to check
/// @return true if the opcode modifies VM state (frames, trail, choice points)
///
/// Side-effecting opcodes are important for optimization and analysis.
/// They cannot be reordered or eliminated without careful consideration.
inline bool hasSideEffects(Opcode op)
{
	switch (op)
	{
		// Binding creates trail entries
		case Opcode::BIND_VAR:

		// Scope operations modify frame stack
		case Opcode::BEGIN_BLOCK:
		case Opcode::END_BLOCK:
		case Opcode::EXPORT_BINDINGS:

		// Backtracking operations modify choice stack
		case Opcode::TRY:
		case Opcode::TRUST:
		case Opcode::CUT:
		case Opcode::FAIL:

		// RETRY modifies existing choice point
		case Opcode::RETRY:

		// Control flow
		case Opcode::HALT:
			return true;

		default:
			return false;
	}
}

/// @brief Get number of operands for an opcode
/// @param op The opcode
/// @return Number of operands expected
///
/// Used for validation and bytecode disassembly.
inline size_t getOperandCount(Opcode op)
{
	switch (op)
	{
		// 0 operands
		case Opcode::HALT:
		case Opcode::EXPORT_BINDINGS:
		case Opcode::TRUST:
		case Opcode::CUT:
		case Opcode::FAIL:
			return 0;

		// 1 operand
		case Opcode::JUMP:
		case Opcode::BEGIN_BLOCK:
		case Opcode::END_BLOCK:
		case Opcode::TRY:
		case Opcode::RETRY:
		case Opcode::DEBUG_PRINT:
			return 1;

		// 2 operands
		case Opcode::MOVE:
		case Opcode::LOAD_IMM:
		case Opcode::BRANCH_FALSE:
		case Opcode::BIND_VAR:
			return 2;

		// 3 operands
		case Opcode::GET_PART:
		case Opcode::MATCH_HEAD:
		case Opcode::MATCH_LENGTH:
		case Opcode::MATCH_LITERAL:
		case Opcode::SAMEQ:
			return 3;

		default:
			return 0;
	}
}

/// @brief Get human-readable description of opcode
/// @param op The opcode
/// @return A short description string
inline const char* getOpcodeDescription(Opcode op)
{
	switch (op)
	{
		// Data movement
		case Opcode::MOVE:
			return "Copy value between registers";
		case Opcode::LOAD_IMM:
			return "Load immediate constant";

		// Introspection
		case Opcode::GET_PART:
			return "Extract part of expression";

		// Pattern matching
		case Opcode::MATCH_HEAD:
			return "Match head and branch on failure";
		case Opcode::MATCH_LENGTH:
			return "Match argument count and branch on failure";
		case Opcode::MATCH_LITERAL:
			return "Match literal value and branch on failure";

		// Comparison
		case Opcode::SAMEQ:
			return "Test structural equality";

		// Binding
		case Opcode::BIND_VAR:
			return "Bind pattern variable";

		// Control flow
		case Opcode::JUMP:
			return "Unconditional jump";
		case Opcode::BRANCH_FALSE:
			return "Jump if condition is false";
		case Opcode::HALT:
			return "Stop execution";

		// Scope management
		case Opcode::BEGIN_BLOCK:
			return "Begin lexical scope";
		case Opcode::END_BLOCK:
			return "End lexical scope";
		case Opcode::EXPORT_BINDINGS:
			return "Export bindings to result";

		// Backtracking
		case Opcode::TRY:
			return "Create choice point (first alternative)";
		case Opcode::RETRY:
			return "Update choice point (middle alternative)";
		case Opcode::TRUST:
			return "Remove choice point (last alternative)";
		case Opcode::CUT:
			return "Commit to current choice";
		case Opcode::FAIL:
			return "Trigger backtracking";

		// Debug
		case Opcode::DEBUG_PRINT:
			return "Print debug information";

		default:
			return "Unknown opcode";
	}
}

//=============================================================================
// Register and Operand Types
//=============================================================================

/// Expression register index (%e0, %e1, %e2, ...)
/// Registers hold Expr values during pattern matching
using ExprRegIndex = size_t;

/// Boolean register index (%b0, %b1, %b2, ...)
/// Registers hold boolean values (comparison results, etc.)
using BoolRegIndex = size_t;

/// Label (instruction index) for jumps
/// Labels are resolved to actual instruction indices during compilation
using Label = size_t;

/// Identifier (variable name, e.g., "Global`x")
/// Used for BIND_VAR to name pattern variables
using Ident = std::string;

/// Immediate expression value
/// Used for LOAD_IMM and pattern constants
using ImmExpr = Expr;

//=============================================================================
// Operand Wrapper Types
//=============================================================================
// These wrapper structs allow std::variant to distinguish between different
// uses of the same underlying type (e.g., ExprRegIndex vs Label)

/// Expression register operand wrapper
struct ExprRegOp
{
	ExprRegIndex v;
	bool operator==(const ExprRegOp& other) const { return v == other.v; }
	bool operator!=(const ExprRegOp& other) const { return v != other.v; }
};

/// Boolean register operand wrapper
struct BoolRegOp
{
	BoolRegIndex v;
	bool operator==(const BoolRegOp& other) const { return v == other.v; }
	bool operator!=(const BoolRegOp& other) const { return v != other.v; }
};

/// Label operand wrapper
struct LabelOp
{
	Label v;
	bool operator==(const LabelOp& other) const { return v == other.v; }
	bool operator!=(const LabelOp& other) const { return v != other.v; }
};

/// Immediate integer (mint) operand wrapper
struct ImmMint
{
	mint v;
	bool operator==(const ImmMint& other) const { return v == other.v; }
	bool operator!=(const ImmMint& other) const { return v != other.v; }
};

//=============================================================================
// Operand Variant and Helpers
//=============================================================================

/// Operand: A variant that can hold any operand type
///
/// Possible types:
/// - std::monostate: No operand (for 0-operand instructions)
/// - ExprRegOp: Expression register (%e0, %e1, ...)
/// - BoolRegOp: Boolean register (%b0, %b1, ...)
/// - LabelOp: Jump target label (L0, L1, ...)
/// - Ident: Variable name string ("Global`x")
/// - ImmExpr: Immediate Expr constant (for LOAD_IMM)
/// - ImmMint: Immediate integer (for part indices, etc.)
using Operand = std::variant<std::monostate, ExprRegOp, BoolRegOp, LabelOp, Ident, ImmExpr, ImmMint>;

/// Helper: Create an expression register operand
inline Operand OpExprReg(ExprRegIndex r)
{
	return Operand { ExprRegOp { r } };
}

/// Helper: Create a boolean register operand
inline Operand OpBoolReg(BoolRegIndex b)
{
	return Operand { BoolRegOp { b } };
}

/// Helper: Create a label operand
inline Operand OpLabel(Label L)
{
	return Operand { LabelOp { L } };
}

/// Helper: Create an identifier operand
inline Operand OpIdent(const std::string& s)
{
	return Operand { s };
}

/// Helper: Create an immediate Expr operand
inline Operand OpImm(const Expr& e)
{
	return Operand { e };
}

/// Helper: Create an immediate mint operand
inline Operand OpImm(mint v)
{
	return Operand { ImmMint { v } };
}

//=============================================================================
// Utility Functions
//=============================================================================

/// @brief Get the name of an opcode as a string
/// @param op The opcode
/// @return String representation (e.g., "MOVE", "MATCH_HEAD")
const char* opcodeName(Opcode op);

/// @brief Convert an operand to a string representation
/// @param op The operand to convert
/// @return Human-readable string (e.g., "%e0", "L5", "\"Global`x\"")
std::string operandToString(const Operand& op);

/// @brief Check if an operand is of a specific type
/// @tparam T The type to check for (e.g., ExprRegOp, LabelOp)
/// @param op The operand to check
/// @return true if the operand holds a value of type T
template <typename T>
bool isOperandType(const Operand& op)
{
	return std::holds_alternative<T>(op);
}

/// @brief Safely extract operand as a specific type
/// @tparam T The type to extract (e.g., ExprRegOp, LabelOp)
/// @param op The operand
/// @return std::optional containing the value if the type matches, nullopt otherwise
///
/// Example:
///   if (auto reg = getOperandAs<ExprRegOp>(operand)) {
///       std::cout << "Register: %e" << reg->v << std::endl;
///   }
template <typename T>
std::optional<T> getOperandAs(const Operand& op)
{
	if (auto* ptr = std::get_if<T>(&op))
		return *ptr;
	return std::nullopt;
}

}; // namespace PatternMatcher