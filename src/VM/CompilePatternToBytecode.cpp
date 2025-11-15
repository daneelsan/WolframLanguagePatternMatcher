#include "VM/CompilePatternToBytecode.h"

#include "VM/PatternBytecode.h"
#include "VM/Opcode.h"

#include "AST/MExpr.h"
#include "AST/MExprPatternTools.h"

#include "Expr.h"
#include "Logger.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace PatternMatcher
{

/*===========================================================================
Pattern Compilation to Bytecode

This file compiles pattern expressions (like x_, {a_, b_}, f[_Integer], etc.)
into bytecode instructions that can be executed by the VirtualMachine.

Key Concepts:
- Patterns are compiled to a sequence of instructions
- Instructions test properties (head, length, equality) and bind variables
- Control flow uses labels and jumps (like assembly code)
- Alternatives use backtracking (choice points) like Prolog

Register Model:
- %e0: Always holds the current expression being matched
- %e1, %e2, ...: Temporary registers for subexpressions and bindings
- %b0: Final match result (true/false)
- %b1, %b2, ...: Temporary boolean registers

Label Model:
- successLabel: Jump here when pattern matches
- failLabel: Jump here when pattern fails
- innerFail: Local failure within a subpattern (may unwind blocks)
===========================================================================*/

/*---------------------------------------------------------------------------
CompilerState: The Central State Manager

Manages all compilation state:
1. Register allocation (expr and bool registers)
2. Label generation and binding
3. Bytecode instruction emission
4. Lexical environment tracking (variable name → register mapping)
5. Block stack (for nested scopes)
---------------------------------------------------------------------------*/
struct CompilerState
{
	std::shared_ptr<PatternBytecode> out = std::make_shared<PatternBytecode>();

	// Register allocation counters
	// Note: %e0 and %b0 are reserved (see comment above)
	ExprRegIndex nextExprReg = 1; // Next available expr register
	BoolRegIndex nextBoolReg = 1; // Next available bool register
	Label nextLabel = 0; // Next available label number

	// Maps variable names (e.g., "Global`x") to the register holding their value
	// This allows detecting repeated variables: f[x_, x_] requires both x's to match
	std::unordered_map<std::string, ExprRegIndex> lexical;

	// Stack of currently open blocks (for proper nesting)
	// Used by beginBlock/endBlock to ensure balanced BEGIN_BLOCK/END_BLOCK
	std::vector<Label> blockStack;

	//=========================//
	//  Register allocators
	//=========================//

	/// Allocate a new expression register (%e1, %e2, ...)
	ExprRegIndex allocExprReg() { return nextExprReg++; }

	/// Allocate a new boolean register (%b1, %b2, ...)
	BoolRegIndex allocBoolReg() { return nextBoolReg++; }

	//=========================//
	//  Label management
	//=========================//

	/// Create a new unique label number
	/// Labels are resolved to instruction indices later by the bytecode
	Label newLabel() { return nextLabel++; }

	/// Bind a label to the current instruction position
	/// Like "L3:" in assembly - marks where jumps to L3 should go
	void bindLabel(Label L) { out->addLabel(L); }

	//=========================//
	//  Instruction emission
	//=========================//

	/// Emit a single bytecode instruction with operands
	/// Example: emit(Opcode::MOVE, { OpExprReg(1), OpExprReg(0) })
	///   → Generates: MOVE %e1, %e0
	void emit(Opcode op, std::initializer_list<Operand> ops = {}) { out->push_instr(op, ops); }

	/// Begin a new lexical scope block
	/// Emits BEGIN_BLOCK instruction and pushes label onto block stack
	/// Blocks create frames at runtime for variable bindings
	void beginBlock(Label L)
	{
		out->addLabel(L); // Bind label to current position
		blockStack.push_back(L); // Track for proper nesting
		emit(Opcode::BEGIN_BLOCK, { OpLabel(L) });
	}

	/// Close a lexical scope block
	/// Emits END_BLOCK instruction and pops from block stack
	/// If L is not the topmost block, unwinds inner blocks first (error recovery)
	void endBlock(Label L)
	{
		if (blockStack.empty())
		{
			PM_WARNING("endBlock(", L, "): no open blocks to end");
			emit(Opcode::END_BLOCK, { OpLabel(L) });
			return;
		}

		// Common case: closing the most recently opened block
		if (blockStack.back() == L)
		{
			blockStack.pop_back();
			emit(Opcode::END_BLOCK, { OpLabel(L) });
			return;
		}

		// Error recovery: unwind inner blocks to find L
		PM_DEBUG("endBlock: unwinding blocks to find L", L);
		while (!blockStack.empty() && blockStack.back() != L)
		{
			Label inner = blockStack.back();
			blockStack.pop_back();
			PM_DEBUG("endBlock: emitting END_BLOCK for inner label ", inner);
			emit(Opcode::END_BLOCK, { OpLabel(inner) });
		}

		if (blockStack.empty())
		{
			PM_WARNING("endBlock: label not found while unwinding. Emitting END_BLOCK(", L, ") anyway.");
			emit(Opcode::END_BLOCK, { OpLabel(L) });
			return;
		}

		blockStack.pop_back();
		emit(Opcode::END_BLOCK, { OpLabel(L) });
	}

	/// Emit a jump to success if this pattern is top-level
	///
	/// Why isTopLevel matters:
	/// - Top-level patterns (isTopLevel=true): Must explicitly jump to success
	///   Example: In "x_", after matching, jump to success block
	///
	/// - Nested patterns (isTopLevel=false): Fall through to next instruction
	///   Example: In "f[x_, y_]", after matching x_, continue to match y_
	///
	/// Special case: In alternatives, each branch is treated as top-level
	/// because it must commit by jumping to success, not fall through
	void emitSuccessJumpIfTopLevel(Label successLabel, bool isTopLevel)
	{
		if (isTopLevel)
		{
			emit(Opcode::JUMP, { OpLabel(successLabel) });
		}
	}
};

// Forward declaration for mutual recursion
static void compilePatternRec(CompilerState& st, std::shared_ptr<MExpr> mexpr, Label successLabel, Label failLabel,
							  bool isTopLevel);

/*---------------------------------------------------------------------------
compileLiteralMatch: Match against constant values

Compiles patterns like:
- 5 (integer literal)
- "hello" (string literal)
- Pi (symbol literal)

Generated code:
  MATCH_LITERAL %e0, <literal>, failLabel
  [JUMP successLabel]  ; if isTopLevel=true

If the input doesn't match the literal, jumps to failLabel.
Otherwise continues (or jumps to success if top-level).
---------------------------------------------------------------------------*/
static void compileLiteralMatch(CompilerState& st, std::shared_ptr<MExpr> mexpr, Label successLabel, Label failLabel,
								bool isTopLevel)
{
	// Compare input expression (%e0) with literal
	// On mismatch, MATCH_LITERAL jumps to failLabel
	st.emit(Opcode::MATCH_LITERAL, { OpExprReg(0), OpImm(mexpr->getExpr()), OpLabel(failLabel) });

	// If top-level, explicitly jump to success
	// If nested, fall through to next pattern check
	st.emitSuccessJumpIfTopLevel(successLabel, isTopLevel);
}

/*---------------------------------------------------------------------------
compileBlank: Match any expression with optional head

Compiles patterns like:
- _ (blank - matches any single expression)
- _Integer (blank with head constraint - matches 1, 2, 3, but not 1.5)
- _Real (matches 1.5, 2.0, but not 1)

Generated code for _Integer:
  MATCH_HEAD %e0, Integer, failLabel
  [JUMP successLabel]  ; if isTopLevel=true

The blank itself (_) always matches, so it just jumps to success.
---------------------------------------------------------------------------*/
static void compileBlank(CompilerState& st, std::shared_ptr<MExprNormal> mexpr, Label successLabel, Label failLabel,
						 bool isTopLevel)
{
	if (mexpr->length() == 1)
	{
		// Blank[f] → check if Head[input] == f
		// Example: _Integer checks if Head[5] == Integer
		Expr headExpr = mexpr->part(1)->getExpr();
		st.emit(Opcode::MATCH_HEAD, { OpExprReg(0), OpImm(headExpr), OpLabel(failLabel) });
	}
	// Otherwise, Blank[] (plain _) matches any single expression - no check needed

	st.emitSuccessJumpIfTopLevel(successLabel, isTopLevel);
}

/*---------------------------------------------------------------------------
compilePattern: Match and Bind Named Patterns

Compiles patterns like:
- x_ (match anything and bind to variable x)
- x_Integer (match integer and bind to x)
- f[x_, x_] (both x's must match the same value - "repeated variable")

Two cases:

1. FIRST OCCURRENCE (x not yet bound):
   - Compile the subpattern (e.g., _Integer)
   - If it matches, bind x to the current value
   - Track x in lexical environment for later occurrences

2. REPEATED VARIABLE (x already bound):
   - Check if current value equals previously bound value
   - If not equal, fail
   - Then compile the subpattern

Example bytecode for x_Integer (first occurrence):
  MATCH_HEAD %e0, Integer, innerFail    ; Check if integer
  MOVE %e3, %e0                         ; Copy value to register
  BIND_VAR "Global`x", %e3              ; Runtime binding
  JUMP successLabel                     ; Success!
innerFail:
  JUMP outerFail                        ; Subpattern failed

Example bytecode for second x in f[x_, x_]:
  SAMEQ %b1, %e3, %e0                   ; Compare with first x
  JUMP_IF_FALSE %b1, outerFail          ; Must match!
  [continue with subpattern]
---------------------------------------------------------------------------*/
static void compilePattern(CompilerState& st, std::shared_ptr<MExprNormal> mexpr, Label successLabel, Label outerFail,
						   bool isTopLevel)
{
	// Pattern structure: Pattern[symbol, subpattern]
	// Example: Pattern[x, Blank[Integer]] represents x_Integer
	auto parts = mexpr->getChildren();
	if (parts.size() < 2)
	{
		// Malformed pattern → immediate failure
		st.emit(Opcode::JUMP, { OpLabel(outerFail) });
		return;
	}

	// Extract variable name and subpattern
	auto symM = std::static_pointer_cast<MExprSymbol>(parts[0]);
	std::string lexName = symM->getLexicalName(); // e.g., "Global`x"
	auto subp = parts[1]; // e.g., Blank[Integer]

	auto it = st.lexical.find(lexName);
	if (it != st.lexical.end())
	{
		// ============================================================
		// REPEATED VARIABLE: x already bound earlier in pattern
		// Example: f[x_, x_] - second x must equal first x
		// ============================================================

		ExprRegIndex storedReg = it->second; // Register holding first x's value
		BoolRegIndex b = st.allocBoolReg();

		// Check if current value (%e0) equals stored value
		st.emit(Opcode::SAMEQ, { OpBoolReg(b), OpExprReg(storedReg), OpExprReg(0) });

		// If not equal, pattern fails
		st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(b), OpLabel(outerFail) });

		// If equal, still need to check the subpattern constraint
		// Example: In f[x_Integer, x_Real], even if both x's are equal,
		// we still need to verify the type constraints
		compilePatternRec(st, subp, successLabel, outerFail, false);

		st.emitSuccessJumpIfTopLevel(successLabel, isTopLevel);
	}
	else
	{
		// ============================================================
		// FIRST OCCURRENCE: x not yet bound
		// Strategy: Check subpattern first, THEN bind
		// ============================================================

		// Create a local failure label for the subpattern
		Label innerFail = st.newLabel();

		// Compile the subpattern (e.g., _Integer)
		// If it fails, jump to innerFail (not outerFail directly)
		compilePatternRec(st, subp, successLabel, innerFail, false);

		// Subpattern succeeded. Now bind the variable
		ExprRegIndex bindReg = st.allocExprReg();
		st.emit(Opcode::MOVE, { OpExprReg(bindReg), OpExprReg(0) }); // Copy value
		st.lexical[lexName] = bindReg; // Track for repeated variable detection

		// Runtime binding: Store in frame for later retrieval
		st.emit(Opcode::BIND_VAR, { OpIdent(lexName), OpExprReg(bindReg) });

		// Success path: jump to success or past failure handler
		Label afterFailHandler = st.newLabel();
		st.emit(Opcode::JUMP, { OpLabel(isTopLevel ? successLabel : afterFailHandler) });

		// ============================================================
		// FAILURE HANDLER: Subpattern didn't match
		// ============================================================
		st.bindLabel(innerFail);
		st.emit(Opcode::JUMP, { OpLabel(outerFail) }); // Propagate failure upward

		// Continuation point for successful binding (non-top-level only)
		st.bindLabel(afterFailHandler);
	}
}

/*---------------------------------------------------------------------------
compileAlternatives: Match Any of Several Patterns (Backtracking)

Compiles patterns like:
- _Real | _Integer (match either a real or an integer)
- x_Integer | x_Real (bind x to either an integer or a real)
- f[x_] | g[y_] (match either f with one arg or g with one arg)

Uses choice points and backtracking (like Prolog):
1. TRY creates a choice point with saved state
2. If alternative fails, FAIL triggers backtracking to next alternative
3. RETRY updates choice point to point to subsequent alternative
4. TRUST removes choice point before trying last alternative

Generated code for p1 | p2 | p3:

  TRY L_p2                 ; Create choice point → if fail, try p2
L_p1:
  [compile p1]
  JUMP success             ; p1 matched!
L_p1_fail:
  FAIL                     ; Backtrack to p2

L_p2:
  RETRY L_p3               ; Update choice point → if fail, try p3
  [compile p2]
  JUMP success             ; p2 matched!
L_p2_fail:
  FAIL                     ; Backtrack to p3

L_p3:
  TRUST                    ; Remove choice point (last alternative)
  [compile p3]
  JUMP success OR fail     ; Either matches or whole pattern fails

NOTE: Each alternative gets a fresh lexical environment!
In x_Integer | x_Real, both alternatives have their own "x" binding.
This prevents the second alternative from thinking x is already bound.
---------------------------------------------------------------------------*/
static void compileAlternatives(CompilerState& st, std::shared_ptr<MExprNormal> mexpr, Label successLabel,
								Label failLabel, bool isTopLevel)
{
	size_t numAlts = mexpr->length();

	// Edge case: No alternatives → immediate failure
	if (numAlts == 0)
	{
		st.emit(Opcode::JUMP, { OpLabel(failLabel) });
		return;
	}

	// Edge case: Single alternative → no choice point needed
	if (numAlts == 1)
	{
		auto alt = mexpr->part(1);
		compilePatternRec(st, alt, successLabel, failLabel, isTopLevel);
		return;
	}

	// Save the lexical environment before processing alternatives
	// Each alternative needs a fresh start - variables bound in one
	// alternative shouldn't affect variable detection in others
	//
	// Example: In x_Integer | x_Real:
	// - First alternative adds x → %e3 to st.lexical
	// - Without restoration, second alternative sees x as "repeated variable"
	// - With restoration, second alternative treats x as "first occurrence"
	auto savedLexical = st.lexical;

	// Create labels for each alternative's entry point
	std::vector<Label> altLabels;
	for (size_t i = 0; i < numAlts; ++i)
	{
		altLabels.push_back(st.newLabel());
	}

	// ============================================================
	// FIRST ALTERNATIVE: Create choice point with TRY
	// ============================================================
	// TRY saves the current state (registers, frames, trail)
	// and records where to jump on backtrack (altLabels[1])
	st.emit(Opcode::TRY, { OpLabel(altLabels[1]) });
	st.bindLabel(altLabels[0]);
	{
		auto firstAlt = mexpr->part(1);
		Label firstFail = st.newLabel();

		// Compile first alternative as top-level (must jump to success)
		compilePatternRec(st, firstAlt, successLabel, firstFail, true);

		// If first alternative fails, trigger backtracking
		st.bindLabel(firstFail);
		st.emit(Opcode::FAIL, {}); // VM will restore state and jump to altLabels[1]
	}

	// ============================================================
	// MIDDLE ALTERNATIVES: Update choice point with RETRY
	// ============================================================
	// For numAlts=2, this loop doesn't run (goes straight to TRUST)
	// For numAlts=3+, this handles alternatives 2, 3, ..., numAlts-1
	for (size_t i = 1; i < numAlts - 1; ++i)
	{
		// Restore lexical environment for independent variable tracking
		st.lexical = savedLexical;

		st.bindLabel(altLabels[i]);
		// RETRY updates the choice point's next alternative pointer
		// If this alternative fails, backtrack to altLabels[i+1]
		st.emit(Opcode::RETRY, { OpLabel(altLabels[i + 1]) });

		auto alt = mexpr->part(static_cast<mint>(i + 1));
		Label altFail = st.newLabel();

		compilePatternRec(st, alt, successLabel, altFail, true);

		st.bindLabel(altFail);
		st.emit(Opcode::FAIL, {}); // Backtrack to next alternative
	}

	// ============================================================
	// LAST ALTERNATIVE: Remove choice point with TRUST
	// ============================================================
	// TRUST commits to this alternative - no more backtracking possible
	// If this fails, the entire pattern fails (no more alternatives to try)
	st.lexical = savedLexical; // Restore environment one last time

	st.bindLabel(altLabels[numAlts - 1]);
	st.emit(Opcode::TRUST, {}); // Remove choice point from stack
	{
		auto lastAlt = mexpr->part(static_cast<mint>(numAlts));
		// Last alternative: on success → jump to successLabel
		//                   on failure → jump to failLabel (no backtracking!)
		compilePatternRec(st, lastAlt, successLabel, failLabel, true);
	}
}

/*---------------------------------------------------------------------------
compileNormal: Match Structured Expressions

Compiles patterns like:
- f[x_, y_] (matches f with 2 arguments)
- {a_, b_, c_} (matches a list with 3 elements)
- Plus[x_, y_] (matches Plus with 2 arguments)

Strategy:
1. Check argument count (length)
2. Check head (e.g., f, List, Plus)
3. For each argument, recursively match its pattern
4. Restore %e0 between argument matches (since recursive calls modify it)

Generated code for f[x_, y_]:

L_block:
  BEGIN_BLOCK L_block         ; Create scope for temps
  MATCH_LENGTH %e0, 2, innerFail
  MATCH_HEAD %e0, f, innerFail
  MOVE %e1, %e0               ; Save original expression

  ; Match first argument (x_)
  GET_PART %e2, %e0, 1        ; Extract f[...][1]
  MOVE %e0, %e2               ; Put in %e0 for matching
  [compile x_ with innerFail as fail label]
  MOVE %e0, %e1               ; Restore original

  ; Match second argument (y_)
  GET_PART %e3, %e0, 2        ; Extract f[...][2]
  MOVE %e0, %e3
  [compile y_ with innerFail as fail label]
  MOVE %e0, %e1               ; Restore original

  END_BLOCK L_block           ; All matched!
  JUMP afterFailHandler

innerFail:
  JUMP outerFail

afterFailHandler:
  ; Continue...
---------------------------------------------------------------------------*/
static void compileNormal(CompilerState& st, std::shared_ptr<MExprNormal> mexpr, Label successLabel, Label outerFail,
						  bool isTopLevel)
{
	size_t argsLen = mexpr->length();

	// Create a block for temporary registers
	// This creates a runtime frame for variable bindings within this pattern
	Label blockLabel = st.newLabel();
	st.beginBlock(blockLabel);

	// Local failure label - unwinds this block before propagating failure
	Label innerFail = st.newLabel();

	// --- 1. Check argument count ---
	// Example: f[x_, y_] requires exactly 2 arguments
	st.emit(Opcode::MATCH_LENGTH, { OpExprReg(0), OpImm(argsLen), OpLabel(innerFail) });

	mint partStart = 0; // Which part index to start matching from

	// --- 2. Check head equality ---
	auto headMExpr = mexpr->getHead();
	if (headMExpr->symbolQ())
	{
		// Head is a symbol (e.g., f, List, Plus)
		// We can check it directly with MATCH_HEAD
		Expr headExpr = headMExpr->getExpr();
		st.emit(Opcode::MATCH_HEAD, { OpExprReg(0), OpImm(headExpr), OpLabel(innerFail) });

		// Skip matching part(0) since we already checked the head
		partStart = 1;
	}
	// If head is not a symbol (rare), we'd need to match it as part(0)

	// --- 3. Save %e0 before recursive matching ---
	// Each child pattern compilation uses %e0, so we need to preserve the original
	ExprRegIndex rSaved = st.allocExprReg();
	st.emit(Opcode::MOVE, { OpExprReg(rSaved), OpExprReg(0) });

	// --- 4. Match each argument subpattern ---
	for (mint i = partStart; i <= static_cast<mint>(argsLen); ++i)
	{
		// Extract the i-th part into a fresh register
		ExprRegIndex rPart = st.allocExprReg();
		st.emit(Opcode::GET_PART, { OpExprReg(rPart), OpExprReg(0), OpImm(i) });

		// Move part into %e0 for matching (convention: %e0 = current value)
		st.emit(Opcode::MOVE, { OpExprReg(0), OpExprReg(rPart) });

		auto child = mexpr->part(i);
		// Compile child pattern with isTopLevel=false
		// On success, it falls through to next iteration
		// On failure, it jumps to innerFail
		compilePatternRec(st, child, successLabel, innerFail, false);

		// Restore %e0 to original value for next part extraction
		st.emit(Opcode::MOVE, { OpExprReg(0), OpExprReg(rSaved) });
	}

	// All arguments matched successfully!
	st.endBlock(blockLabel);

	// Create continuation point for non-top-level patterns
	Label afterFailHandler = st.newLabel();

	if (!isTopLevel)
	{
		// Fall through to next pattern check
		st.emit(Opcode::JUMP, { OpLabel(afterFailHandler) });
	}
	else
	{
		// Top-level: jump to success
		st.emitSuccessJumpIfTopLevel(successLabel, isTopLevel);
	}

	// ============================================================
	// FAILURE HANDLER: One of the checks failed
	// ============================================================
	st.bindLabel(innerFail);
	st.emit(Opcode::JUMP, { OpLabel(outerFail) }); // Propagate failure

	st.bindLabel(afterFailHandler); // Continuation for nested patterns
}

/*---------------------------------------------------------------------------
compilePatternRec: Main Pattern Compilation Dispatcher

Recursively compiles any pattern expression by dispatching to specialized
handlers based on the pattern's structure.

Pattern types:
- Literal: 5, "hello", Pi
- Symbol: x (treated as literal for now)
- Blank: _, _Integer, _Real
- Pattern: x_, x_Integer (named patterns with binding)
- Alternatives: p1 | p2 | p3 (backtracking choice)
- Normal: f[x_, y_], {a_, b_}, Plus[x_, 1] (structured expressions)

Parameters explained:
@param st: Compiler state (registers, labels, bytecode)
@param mexpr: Pattern to compile (in MExpr form)
@param successLabel: Where to jump on successful match
@param failLabel: Where to jump on failed match
@param isTopLevel:
  - true: Pattern must explicitly jump to successLabel on match
	Used for: top-level patterns, each alternative branch
  - false: Pattern falls through on match (for composition)
	Used for: nested patterns, argument patterns in f[x_, y_]
---------------------------------------------------------------------------*/
static void compilePatternRec(CompilerState& st, std::shared_ptr<MExpr> mexpr, Label successLabel, Label failLabel,
							  bool isTopLevel)
{
	switch (mexpr->getKind())
	{
		case MExpr::Kind::Literal:
		{
			compileLiteralMatch(st, mexpr, successLabel, failLabel, isTopLevel);
			return;
		}
		case MExpr::Kind::Symbol:
		{
			// TODO: Distinguish between symbol patterns and symbol literals
			// For now, treat symbols as literals (e.g., Pi matches only Pi)
			compileLiteralMatch(st, mexpr, successLabel, failLabel, isTopLevel);
			return;
		}
		case MExpr::Kind::Normal:
		{
			auto mexprNormal = std::static_pointer_cast<MExprNormal>(mexpr);
			// Dispatch based on specific pattern type
			if (MExprIsBlank(mexpr))
			{
				compileBlank(st, mexprNormal, successLabel, failLabel, isTopLevel);
			}
			else if (MExprIsPattern(mexpr))
			{
				compilePattern(st, mexprNormal, successLabel, failLabel, isTopLevel);
			}
			else if (MExprIsAlternatives(mexpr))
			{
				compileAlternatives(st, mexprNormal, successLabel, failLabel, isTopLevel);
			}
			else
			{
				// Regular structured expression: f[args...]
				compileNormal(st, mexprNormal, successLabel, failLabel, isTopLevel);
			}
			return;
		}
		default:
			// Unknown/unsupported pattern kind → immediate failure
			st.emit(Opcode::JUMP, { OpLabel(failLabel) });
			return;
	}
}

/*---------------------------------------------------------------------------
CompilePatternToBytecode: Top-Level Entry Point

Compiles a pattern expression into executable bytecode.

Generated structure:
  L0 (entry):
	BEGIN_BLOCK L0
	[pattern compilation]
	END_BLOCK L0

  L1 (fail):
	DEBUG_PRINT "Pattern failed"
	LOAD_IMM %b0, false
	HALT

  L2 (success):
	DEBUG_PRINT "Pattern succeeded"
	SAVE_BINDINGS    ; Capture all variable bindings to result
	LOAD_IMM %b0, true
	HALT

The entry block creates a frame for all pattern bindings.
SAVE_BINDINGS at the end extracts bindings (e.g., x→5) for return.

Returns: Shared pointer to PatternBytecode ready for execution
---------------------------------------------------------------------------*/
std::shared_ptr<PatternBytecode> CompilePatternToBytecode(const Expr& patternExpr)
{
	// Convert Expr to MExpr (internal AST representation)
	auto pattern = MExpr::construct(patternExpr);

	CompilerState st;

	// Allocate standard labels
	Label entryLabel = st.newLabel(); // L0: Entry point
	Label failLabel = st.newLabel(); // L1: Failure handler
	Label successLabel = st.newLabel(); // L2: Success handler

	// ============================================================
	// ENTRY BLOCK: Create frame for pattern matching
	// ============================================================
	st.beginBlock(entryLabel);

	// Compile the pattern with isTopLevel=true
	// On match: jumps to successLabel
	// On fail: jumps to failLabel
	compilePatternRec(st, pattern, successLabel, failLabel, true);

	st.endBlock(entryLabel);

	// ============================================================
	// FAILURE BLOCK: Pattern didn't match
	// ============================================================
	st.bindLabel(failLabel);
	st.emit(Opcode::DEBUG_PRINT, { OpImm(Expr("Pattern failed")) });
	st.emit(Opcode::LOAD_IMM, { OpBoolReg(0), OpImm(false) });
	st.emit(Opcode::HALT, {});

	// ============================================================
	// SUCCESS BLOCK: Pattern matched!
	// ============================================================
	st.bindLabel(successLabel);
	st.emit(Opcode::DEBUG_PRINT, { OpImm(Expr("Pattern succeeded")) });

	// Save all variable bindings from the current frame
	// This extracts bindings like {x→5, y→10} for the caller
	st.emit(Opcode::SAVE_BINDINGS, {});

	st.emit(Opcode::LOAD_IMM, { OpBoolReg(0), OpImm(true) });
	st.emit(Opcode::HALT, {});

	// Finalize bytecode with metadata
	st.out->set_metadata(pattern, st.nextExprReg, st.nextBoolReg, st.lexical);

	return st.out;
}

}; // namespace PatternMatcher