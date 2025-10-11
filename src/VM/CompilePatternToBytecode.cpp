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

	// part(1): symbol name; part(2): subpattern
	auto symM = std::static_pointer_cast<MExprSymbol>(parts[0]);
	std::string lexName = symM->getLexicalName();
	auto subp = parts[1];

	auto it = st.lexical.find(lexName);
	if (it != st.lexical.end())
	{
		// ============================================================
		// REPEATED VARIABLE: Compare with previously bound value
		// ============================================================
		// No need for a block since we're not creating new bindings

		ExprRegIndex storedReg = it->second;
		BoolRegIndex b = st.allocBoolReg();
		st.emit(Opcode::SAMEQ, { OpBoolReg(b), OpExprReg(storedReg), OpExprReg(0) });
		// If comparison fails, jump to outer fail
		st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(b), OpLabel(outerFail) });

		// If comparison succeeds, compile subpattern
		compilePatternRec(st, subp, successLabel, outerFail, false);

		// Success: if top-level, jump to success; otherwise fall through
		if (isTopLevel)
		{
			st.emit(Opcode::JUMP, { OpLabel(successLabel) });
		}
	}
	else
	{
		// ============================================================
		// FIRST OCCURRENCE: Bind variable and match subpattern
		// ============================================================

		// Create a block for this binding
		Label blockLabel = st.newLabel();
		st.beginBlock(blockLabel);

		// Allocate register for the binding
		ExprRegIndex bindReg = st.allocExprReg();
		st.emit(Opcode::MOVE, { OpExprReg(bindReg), OpExprReg(0) });
		st.lexical[lexName] = bindReg;

		// Runtime binding for inspection
		st.emit(Opcode::BIND_VAR, { OpIdent(lexName), OpExprReg(bindReg) });

		// Create inner fail label that will unwind this block
		Label innerFail = st.newLabel();

		// Compile the subpattern (e.g., Blank[], Integer, etc.)
		compilePatternRec(st, subp, successLabel, innerFail, false);

		// If we reach here, subpattern succeeded
		// Close the block normally
		st.endBlock(blockLabel);

		// **KEY FIX**: Create a label to jump past the failure handler
		Label afterFailHandler = st.newLabel();

		// If top-level, jump to success; otherwise jump past failure handler
		if (isTopLevel)
		{
			st.emit(Opcode::JUMP, { OpLabel(successLabel) });
		}
		else
		{
			st.emit(Opcode::JUMP, { OpLabel(afterFailHandler) });
		}

		// ============================================================
		// FAILURE HANDLER: Unwind and propagate failure
		// ============================================================
		st.bindLabel(innerFail);
		// Pop the frame that was created by BEGIN_BLOCK
		st.emit(Opcode::END_BLOCK, { OpLabel(blockLabel) });
		// Jump to outer failure
		st.emit(Opcode::JUMP, { OpLabel(outerFail) });

		// Bind the "after failure handler" label so normal execution continues here
		st.bindLabel(afterFailHandler);
	}
}

/* Helper: compile normal expressions head[arg1, arg2, ...] */
static void compileNormal(CompilerState& st, std::shared_ptr<MExprNormal> mexpr, Label successLabel, Label outerFail,
						  bool isTopLevel)
{
	size_t argsLen = mexpr->length();

	// Create a block for temporaries
	Label blockLabel = st.newLabel();
	st.beginBlock(blockLabel);

	// Inner fail label that will unwind this block
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

	// --- 3. Save %e0 before recursive calls ---
	ExprRegIndex rSaved = st.allocExprReg();
	st.emit(Opcode::MOVE, { OpExprReg(rSaved), OpExprReg(0) });

	// --- 4. Match each argument subpattern ---
	for (mint i = 1; i <= static_cast<mint>(argsLen); ++i)
	{
		// Extract the i-th part
		ExprRegIndex rPart = st.allocExprReg();
		st.emit(Opcode::GET_PART, { OpExprReg(rPart), OpExprReg(0), OpImm(i) });

		// Move part into %e0 for matching
		st.emit(Opcode::MOVE, { OpExprReg(0), OpExprReg(rPart) });

		auto child = mexpr->part(i);
		// Compile child pattern - if it fails, jump to innerFail
		// On success, it falls through to the next iteration
		compilePatternRec(st, child, successLabel, innerFail, false);

		// Restore %e0 for next iteration
		st.emit(Opcode::MOVE, { OpExprReg(0), OpExprReg(rSaved) });
	}

	// All arguments matched successfully
	// Close the block normally
	st.endBlock(blockLabel);

	// **KEY FIX**: Create a label to jump past the failure handler
	Label afterFailHandler = st.newLabel();

	// If top-level, jump to success; otherwise jump past failure handler
	if (isTopLevel)
	{
		st.emit(Opcode::JUMP, { OpLabel(successLabel) });
	}
	else
	{
		st.emit(Opcode::JUMP, { OpLabel(afterFailHandler) });
	}

	// ============================================================
	// FAILURE HANDLER: Unwind and propagate
	// ============================================================
	st.bindLabel(innerFail);
	st.emit(Opcode::END_BLOCK, { OpLabel(blockLabel) });
	st.emit(Opcode::JUMP, { OpLabel(outerFail) });

	// Bind the "after failure handler" label
	st.bindLabel(afterFailHandler);
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

	// Allocate labels
	Label entryLabel = st.newLabel(); // L0
	Label failLabel = st.newLabel(); // L1
	Label successLabel = st.newLabel(); // L2

	// Entry block
	st.beginBlock(entryLabel);

	// Compile the pattern (isTopLevel=true means it will jump to success/fail)
	compilePatternRec(st, pattern, successLabel, failLabel, true);

	// Close entry block
	st.endBlock(entryLabel);

	// ============================================================
	// FAILURE BLOCK
	// ============================================================
	st.bindLabel(failLabel);
	st.beginBlock(failLabel);
	st.emit(Opcode::DEBUG_PRINT, { OpImm(Expr("Pattern failed")) });
	st.emit(Opcode::LOAD_IMM, { OpBoolReg(0), OpImm(false) });
	st.emit(Opcode::HALT, {});
	st.endBlock(failLabel);

	// ============================================================
	// SUCCESS BLOCK
	// ============================================================
	st.bindLabel(successLabel);
	st.beginBlock(successLabel);
	st.emit(Opcode::DEBUG_PRINT, { OpImm(Expr("Pattern succeeded")) });
	st.emit(Opcode::LOAD_IMM, { OpBoolReg(0), OpImm(true) });
	st.emit(Opcode::HALT, {});
	st.endBlock(successLabel);

	st.out->set_metadata(pattern, st.nextExprReg, st.nextBoolReg, st.lexical);
	return st.out;
}

}; // namespace PatternMatcher