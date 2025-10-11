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
Pattern Compilation
===========================================================================*/

/// CompilerState: manages registers, labels, and bytecode emission
struct CompilerState
{
	std::shared_ptr<PatternBytecode> out = std::make_shared<PatternBytecode>();

	ExprRegIndex nextExprReg = 1; // reserve %e0 for input expression
	BoolRegIndex nextBoolReg = 1; // reserve %b0 for final match result
	Label nextLabel = 0;

	std::unordered_map<std::string, ExprRegIndex> lexical; // lexical var -> reg

	std::vector<Label> blockStack; // LIFO stack of open blocks

	//=========================//
	//  Register allocators
	//=========================//
	ExprRegIndex allocExprReg() { return nextExprReg++; }
	BoolRegIndex allocBoolReg() { return nextBoolReg++; }

	//=========================//
	//  Label management
	//=========================//
	Label newLabel() { return nextLabel++; }

	/// Bind a label to the current instruction index (like LLVM block label)
	void bindLabel(Label L) { out->addLabel(L); }

	//=========================//
	//  Instruction emission
	//=========================//
	void emit(Opcode op, std::initializer_list<Operand> ops = {}) { out->push_instr(op, ops); }

	/// Debug helper: emit a labeled block marker and push it on the stack
	void beginBlock(Label L)
	{
		// Record that label L begins at the next instruction index
		out->addLabel(L);
		// push onto the stack
		blockStack.push_back(L);
		// emit a visible BEGIN_BLOCK marker (useful in debug)
		emit(Opcode::BEGIN_BLOCK, { OpLabel(L) });
	}

	/// Close a block. If the given label is not the current open block,
	/// unwind inner blocks until L is found (emit END_BLOCKs), or log an error.
	void endBlock(Label L)
	{
		if (blockStack.empty())
		{
			PM_WARNING("endBlock(", L, "): no open blocks to end");
			// still emit an END_BLOCK marker to keep disassembly readable, but warn
			emit(Opcode::END_BLOCK, { OpLabel(L) });
			return;
		}

		// If L is the top, just pop and emit one END_BLOCK
		if (blockStack.back() == L)
		{
			blockStack.pop_back();
			emit(Opcode::END_BLOCK, { OpLabel(L) });
			return;
		}

		// Otherwise unwind until we find L (or empty)
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
			// never found L
			PM_WARNING("endBlock: label not found while unwinding. Emitting END_BLOCK(", L, ") anyway.");
			emit(Opcode::END_BLOCK, { OpLabel(L) });
			return;
		}

		// found L
		blockStack.pop_back();
		emit(Opcode::END_BLOCK, { OpLabel(L) });
	}
};

/*
 * NEW: compilePatternRec takes both success and fail labels.
 *
 *   - success: label to jump to when this subpattern matches.
 *   - fail: label to jump to when this subpattern fails (will usually unwind).
 *
 * Convention:
 *   - Input expression to match is in %e0 and must not be permanently modified.
 */
static void compilePatternRec(CompilerState& st, std::shared_ptr<MExpr> mexpr, Label successLabel, Label failLabel,
							  bool isTopLevel);

/* Helper: compile literal match: produce a Bool register (caller will handle jump on failure) */
static BoolRegIndex compileLiteralMatch(CompilerState& st, std::shared_ptr<MExpr> mexpr)
{
	// Allocate an expression register for the literal
	ExprRegIndex rLit = st.allocExprReg();
	st.emit(Opcode::LOAD_IMM, { OpExprReg(rLit), OpImm(mexpr->getExpr()) });
	// Compare input expression (%e0) with literal rLit
	BoolRegIndex b = st.allocBoolReg();
	st.emit(Opcode::SAMEQ, { OpBoolReg(b), OpExprReg(0), OpExprReg(rLit) });
	return b;
}

/* Helper: compile Blank (with optional head) -> returns Bool reg */
static BoolRegIndex compileBlank(CompilerState& st, std::shared_ptr<MExprNormal> mexpr)
{
	// Blank[] matches any single expression -> always true.
	if (mexpr->length() == 0)
	{
		// Emit a literal boolean True
		BoolRegIndex b = st.allocBoolReg();
		st.emit(Opcode::LOAD_IMM, { OpBoolReg(b), OpImm(true) });
		return b;
	}

	// Otherwise: Blank[f] → check Head[%e0] == f
	ExprRegIndex rH = st.allocExprReg();
	st.emit(Opcode::GET_HEAD, { OpExprReg(rH), OpExprReg(0) });

	Expr headExpr = mexpr->part(1)->getExpr();
	ExprRegIndex rHeadImm = st.allocExprReg();
	st.emit(Opcode::LOAD_IMM, { OpExprReg(rHeadImm), OpImm(headExpr) });

	BoolRegIndex b = st.allocBoolReg();
	st.emit(Opcode::SAMEQ, { OpBoolReg(b), OpExprReg(rH), OpExprReg(rHeadImm) });

	return b;
}

/* Helper: compile Pattern[sym, patt] */
static void compilePattern(CompilerState& st, std::shared_ptr<MExprNormal> mexpr, Label successLabel, Label outerFail,
						   bool isTopLevel)
{
	// parts: part(1) is symbol (lexical name), part(2) is subpattern
	auto parts = mexpr->getChildren();
	if (parts.size() < 2)
	{
		// malformed -> jump directly to outer fail
		st.emit(Opcode::JUMP, { OpLabel(outerFail) });
		return;
	}

	// Create a block so we can unwind the binding on failure
	Label blockLabel = st.newLabel();
	st.beginBlock(blockLabel); // compile-time push + BEGIN_BLOCK marker

	// inner fail handler that will unwind this block and then jump to outerFail
	Label innerFail = st.newLabel();

	// part(1): symbol name; part(2): subpattern
	auto symM = std::static_pointer_cast<MExprSymbol>(parts[0]); // validated earlier
	std::string lexName = symM->getLexicalName();

	auto it = st.lexical.find(lexName);
	if (it != st.lexical.end())
	{
		// repeated variable: compare current expr (%e0) with stored one
		ExprRegIndex storedReg = it->second;
		BoolRegIndex b = st.allocBoolReg();
		st.emit(Opcode::SAMEQ, { OpBoolReg(b), OpExprReg(storedReg), OpExprReg(0) });
		// jump on false -> innerFail so it will unwind
		st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(b), OpLabel(innerFail) });

		// BUG FIX: After repeated variable check succeeds, we need to:
		// 1. Check the subpattern (if it's not just Blank[])
		// 2. Close the block properly
		// 3. Handle top-level vs non-top-level

		auto subp = parts[1];
		// Compile the subpattern with proper labels
		compilePatternRec(st, subp, successLabel, innerFail, false);
	}
	else
	{
		// first occurrence → bind current expr (%e0)
		ExprRegIndex bindReg = st.allocExprReg();
		st.emit(Opcode::MOVE, { OpExprReg(bindReg), OpExprReg(0) });
		st.lexical[lexName] = bindReg;

		// runtime binding for inspection: BIND_VAR stores to runtime frame
		st.emit(Opcode::BIND_VAR, { OpIdent(lexName), OpExprReg(bindReg) });

		// compile the subpattern with innerFail as failure handler
		auto subp = parts[1];
		compilePatternRec(st, subp, successLabel, innerFail, false);
	}

	// Normal exit from the block: pop the compile-time block and emit END_BLOCK (runtime pop).
	st.endBlock(blockLabel);

	// If this Pattern[...] node is the top-level pattern, emit jump to successLabel;
	// otherwise, just fall through to parent code.
	if (isTopLevel)
	{
		st.emit(Opcode::JUMP, { OpLabel(successLabel) });
	}

	// Unwind handler: on failure inside this block, pop the runtime frame and propagate failure.
	st.bindLabel(innerFail);
	st.emit(Opcode::END_BLOCK, { OpLabel(blockLabel) }); // runtime pop during unwind
	st.emit(Opcode::JUMP, { OpLabel(outerFail) });
}

/* Helper: compile normal expressions head[arg1, arg2, ...] */
static void compileNormal(CompilerState& st, std::shared_ptr<MExprNormal> mexpr, Label successLabel, Label outerFail,
						  bool isTopLevel)
{
	size_t argsLen = mexpr->length();

	// --- create a block for temporaries used while matching this normal expression ---
	Label blockLabel = st.newLabel();
	st.beginBlock(blockLabel); // emits BEGIN_BLOCK and pushes blockLabel on compile-time stack

	// make an inner fail label that will unwind this block and then jump to outerFail
	Label innerFail = st.newLabel();

	// --- 1. Test argument length ---
	BoolRegIndex bLen = st.allocBoolReg();
	st.emit(Opcode::TEST_LENGTH, { OpBoolReg(bLen), OpExprReg(0), OpImm(argsLen) });
	st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(bLen), OpLabel(innerFail) });

	// --- 2. Check head equality ---
	ExprRegIndex rHead = st.allocExprReg();
	st.emit(Opcode::GET_HEAD, { OpExprReg(rHead), OpExprReg(0) });

	Expr headExpr = mexpr->getHead()->getExpr();
	ExprRegIndex rHeadImm = st.allocExprReg();
	st.emit(Opcode::LOAD_IMM, { OpExprReg(rHeadImm), OpImm(headExpr) });

	BoolRegIndex bHeadEq = st.allocBoolReg();
	st.emit(Opcode::SAMEQ, { OpBoolReg(bHeadEq), OpExprReg(rHead), OpExprReg(rHeadImm) });
	st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(bHeadEq), OpLabel(innerFail) });

	// --- 3. Save r0 before recursive calls ---
	ExprRegIndex rSaved = st.allocExprReg();
	st.emit(Opcode::MOVE, { OpExprReg(rSaved), OpExprReg(0) });

	// --- 4. Compile each argument subpattern (failures goto innerFail) ---
	for (mint i = 1; i <= static_cast<mint>(argsLen); ++i)
	{
		ExprRegIndex rPart = st.allocExprReg();
		st.emit(Opcode::GET_PART, { OpExprReg(rPart), OpExprReg(0), OpImm(i) });

		// Move part into input register (%e0) for subpattern compilation
		st.emit(Opcode::MOVE, { OpExprReg(0), OpExprReg(rPart) });

		auto child = mexpr->part(i);
		// BUG FIX: For non-top-level subpatterns, pass successLabel correctly
		// but they should NOT jump to success (fall through instead)
		compilePatternRec(st, child, successLabel, innerFail, false);

		// Restore input register
		st.emit(Opcode::MOVE, { OpExprReg(0), OpExprReg(rSaved) });
	}

	// --- Success path: pop the compile-time block and emit END_BLOCK (runtime pop) ---
	st.endBlock(blockLabel); // will emit END_BLOCK and pop blockLabel from compile-time stack

	// If this Normal node is the top-level pattern, emit jump to successLabel;
	// otherwise, just fall through to parent code.
	if (isTopLevel)
	{
		st.emit(Opcode::JUMP, { OpLabel(successLabel) });
	}

	// --- Unwind handler for failures inside this block ---
	st.bindLabel(innerFail);
	st.emit(Opcode::END_BLOCK, { OpLabel(blockLabel) }); // runtime unwind pop
	st.emit(Opcode::JUMP, { OpLabel(outerFail) });
}

/* Recursive compiler dispatcher */
static void compilePatternRec(CompilerState& st, std::shared_ptr<MExpr> mexpr, Label successLabel, Label failLabel,
							  bool isTopLevel)
{
	switch (mexpr->getKind())
	{
		case MExpr::Kind::Literal:
		{
			BoolRegIndex b = compileLiteralMatch(st, mexpr);
			st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(b), OpLabel(failLabel) });
			if (isTopLevel)
			{
				st.emit(Opcode::JUMP, { OpLabel(successLabel) });
			}
			// else: fall through (success)
			return;
		}
		case MExpr::Kind::Symbol:
		{
			// Treat as literal match for now
			BoolRegIndex b = compileLiteralMatch(st, mexpr);
			st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(b), OpLabel(failLabel) });
			if (isTopLevel)
			{
				st.emit(Opcode::JUMP, { OpLabel(successLabel) });
			}
			// else: fall through (success)
			return;
		}
		case MExpr::Kind::Normal:
		{
			auto mexprNormal = std::static_pointer_cast<MExprNormal>(mexpr);
			if (MExprIsBlank(mexpr))
			{
				BoolRegIndex b = compileBlank(st, mexprNormal);
				st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(b), OpLabel(failLabel) });
				if (isTopLevel)
				{
					st.emit(Opcode::JUMP, { OpLabel(successLabel) });
				}
				// else: fall through (success)
				return;
			}
			else if (MExprIsPattern(mexpr))
			{
				compilePattern(st, mexprNormal, successLabel, failLabel, isTopLevel);
				return;
			}
			else
			{
				compileNormal(st, mexprNormal, successLabel, failLabel, isTopLevel);
				return;
			}
		}
		default:
			// Unknown kind: immediate failure
			st.emit(Opcode::JUMP, { OpLabel(failLabel) });
			return;
	}
}

// Entry point for compiling a top-level pattern
std::shared_ptr<PatternBytecode> CompilePatternToBytecode(const Expr& patternExpr)
{
	auto pattern = MExpr::construct(patternExpr);

	CompilerState st;

	// Label allocation
	Label entryLabel = st.newLabel();
	Label failLabel = st.newLabel();
	Label successLabel = st.newLabel();

	// Entry block
	st.beginBlock(entryLabel);

	// Compile the pattern; we pass success and fail labels.
	compilePatternRec(st, pattern, successLabel, failLabel, true);

	// Close entry block (pop any compile-time block marker)
	st.endBlock(entryLabel);

	// Failure block
	st.bindLabel(failLabel);
	st.beginBlock(failLabel);
	st.emit(Opcode::DEBUG_PRINT, { OpImm(Expr("Pattern failed")) });
	st.emit(Opcode::LOAD_IMM, { OpBoolReg(0), OpImm(false) }); // load false into %b0
	st.emit(Opcode::HALT, {});
	st.endBlock(failLabel);

	// Success block
	st.bindLabel(successLabel);
	st.beginBlock(successLabel);
	st.emit(Opcode::DEBUG_PRINT, { OpImm(Expr("Pattern succeeded")) });
	st.emit(Opcode::LOAD_IMM, { OpBoolReg(0), OpImm(true) }); // load true into %b0
	st.emit(Opcode::HALT, {});
	st.endBlock(successLabel);

	st.out->set_metadata(pattern, st.nextExprReg, st.nextBoolReg, st.lexical);
	return st.out;
}

}; // namespace PatternMatcher