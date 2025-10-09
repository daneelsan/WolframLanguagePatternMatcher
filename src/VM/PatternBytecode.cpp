#include "VM/PatternBytecode.h"
#include "VM/Opcode.h"

#include "AST/MExpr.h"
#include "AST/MExprPatternTools.h"

#include "Embeddable.h"
#include "Expr.h"

#include <initializer_list>
#include <memory>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

namespace PatternMatcher
{
std::string PatternBytecode::toString() const
{
	std::ostringstream out;

	auto formatLabel = [&](Label L) {
		auto it = labelMap.find(L);
		if (it != labelMap.end())
			out << "\nL" << L << ":\n";
	};

	out << std::left;
	size_t width = 18; // align operands roughly

	for (size_t i = 0; i < _instrs.size(); ++i)
	{
		// Check if this instruction is the start of a label
		for (auto& kv : labelMap)
		{
			if (kv.second == i)
			{
				out << "\nL" << kv.first << ":\n";
				break;
			}
		}

		// Print opcode and operands
		out << std::setw(4) << i << "  " << std::setw(width) << opcodeName(_instrs[i].opcode);

		if (!_instrs[i].ops.empty())
		{
			bool first = true;
			for (auto& op : _instrs[i].ops)
			{
				if (!first)
					out << ", ";
				out << operandToString(op);
				first = false;
			}
		}
		out << "\n";
	}

	out << "\n----------------------------------------\n";
	out << "Expr registers: " << exprRegisterCount << ", Bool registers: " << boolRegisterCount << "\n";

	if (!lexicalMap.empty())
	{
		out << "Lexical bindings:\n";
		for (auto& p : lexicalMap)
			out << "  " << std::setw(12) << p.first << " → %e" << p.second << "\n";
	}

	return out.str();
}

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

	/// Debug helper: emit a labeled block marker
	void beginBlock(Label L)
	{
		out->addLabel(L);
		emit(Opcode::BEGIN_BLOCK, { OpLabel(L) });
	}

	void endBlock(Label L) { emit(Opcode::END_BLOCK, { OpLabel(L) }); }
};

/*
 * compilePattern returns a boolean register index that contains whether the
 * pattern matched when executed with the input expression in r0.
 *
 * Convention:
 *   - Input expression to match is in r0.
 *   - compilePattern will allocate registers as needed.
 *   - compilePattern will not modify r0 (input expression).
 */
static void compilePatternRec(CompilerState& st, std::shared_ptr<MExpr> mexpr, Label failLabel);

/* Helper: compile literal match: produce bool reg */
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

/* Helper: compile Blank (with optional head) */
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
static void compilePattern(CompilerState& st, std::shared_ptr<MExprNormal> mexpr, Label failLabel)
{
	// parts: part(1) is symbol (lexical name), part(2) is subpattern
	auto parts = mexpr->getChildren();
	if (parts.size() < 2)
	{
		// malformed -> jump directly to failLabel
		st.emit(Opcode::JUMP, { OpLabel(failLabel) });
		return;
	}

	// part(1): symbol name; part(2): subpattern
	auto symM = std::static_pointer_cast<MExprSymbol>(parts[0]); // This was validated by MExprIsPattern
	std::string lexName = symM->getLexicalName();

	auto it = st.lexical.find(lexName);
	if (it != st.lexical.end())
	{
		// repeated variable: compare current expr (%e0) with stored one
		ExprRegIndex storedReg = it->second;
		BoolRegIndex b = st.allocBoolReg();
		st.emit(Opcode::SAMEQ, { OpBoolReg(b), OpExprReg(storedReg), OpExprReg(0) });
		st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(b), OpLabel(failLabel) });
		return;
	}

	// first occurrence → bind current expr (%e0)
	ExprRegIndex bindReg = st.allocExprReg();
	st.emit(Opcode::MOVE, { OpExprReg(bindReg), OpExprReg(0) });
	st.lexical[lexName] = bindReg;

	st.emit(Opcode::BIND_VAR, { OpIdent(lexName), OpExprReg(bindReg) });

	// recursively compile subpattern
	auto subp = parts[1];
	compilePatternRec(st, subp, failLabel);
}

/* Helper: compile PatternTest[patt, test] */
/*static BoolRegIndex compilePatternTest(CompilerState& st, std::shared_ptr<MExprNormal> mexpr, Label failLabel)
{
	// mexpr children: [subpatt, testExpr]
	auto parts = mexpr->getChildren();
	auto subp = parts[0];
	auto testExpr = parts[1]->getExpr();
	// compile subpattern first; result in some boolean reg bsub
	BoolRegIndex bsub = compilePatternRec(st, subp);
	// create result bool reg bres := bsub AND predicate(input)
	// We'll implement PATTERN_TEST which evaluates predicate on input
	BoolRegIndex bres = st.allocBoolReg();
	// PATTERN_TEST bres, predicateExpr
	st.emit(Opcode::PATTERN_TEST, { OpBoolReg(bres), OpImm(testExpr) });
	// We need to AND bsub and bres; since we don't have AND op, do: if bsub false then false, else bres.
	Label lblDone = static_cast<Label>(st.out->getInstructions().size() + 3); // approximate label
	Label lblElse = static_cast<Label>(st.out->getInstructions().size() + 1);

	// if not bsub jump to set false
	st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(bsub), OpLabel(lblDone) });
	// else keep bres as result (result already in bres)
	st.emit(Opcode::JUMP, { OpLabel(lblDone) });
	// label done: nothing to do
	// NOTE: this uses labels that are approximate; we will patch labels after generation.
	// Simpler: return bres (caller may combine). For now just return bres and assume caller handles sequencing.
	return bres;
}*/

/* Helper: compile Except[notPatt] or Except[notPatt, patt] */
/*static BoolRegIndex compileExcept(CompilerState& st, std::shared_ptr<MExprNormal> mexpr)
{
	auto parts = mexpr->getChildren();
	// compile notPatt -> bnot
	BoolRegIndex bnot = compilePatternRec(st, parts[0]);
	// invert
	BoolRegIndex bout = st.allocBoolReg();
	st.emit(Opcode::NOT, { OpBoolReg(bout), OpBoolReg(bnot) });
	if (mexpr->length() == 2)
	{
		// compile maybePatt and AND with bout: result = bout AND maybe
		BoolRegIndex b2 = compilePatternRec(st, parts[1]);
		// Simplest: result := b2 (this is not accurate boolean AND). For brevity return b2 (TODO combine)
		// TODO: Implement proper boolean AND combining
		return b2;
	}
	return bout;
}*/

/* Helper: compile Alternatives (p1 | p2 | ... ) */
/*
static BoolRegIndex compileAlternatives(CompilerState& st, std::shared_ptr<MExprNormal> mexpr)
{
	auto alts = mexpr->getChildren();
	BoolRegIndex result = st.allocBoolReg();

	// naive: evaluate each alt; if any true -> result true
	// We'll allocate a temp bool reg for each alt, and then chain JUMP_IF_TRUE
	Label startLabel = static_cast<Label>(st.out->getInstructions().size());
	Label endLabel = -1; // to patch later

	// initialize result := False (by comparing False and True)
	ExprRegIndex rF = st.allocExprReg();
	st.emit(Opcode::LOAD_IMM, { OpExprReg(rF), OpImm(Expr::ToExpression("False")) });
	ExprRegIndex rT = st.allocExprReg();
	st.emit(Opcode::LOAD_IMM, { OpExprReg(rT), OpImm(Expr::ToExpression("True")) });
	st.emit(Opcode::SAMEQ, { OpBoolReg(result), OpExprReg(rF), OpExprReg(rT) }); // result = false

	// For each alternative
	std::vector<Label> jumpToEndPatches;
	for (auto& alt : alts)
	{
		BoolRegIndex balt = compilePatternRec(st, alt);
		// if balt true -> set result true and jump to end
		// Set result := balt (we need a MOVE-like bool op; we only have NOT and SAMEQ; naive set via control)
		// We'll implement: JUMP_IF_TRUE balt -> set result true (by loading True) and jump to final
		Label afterAlt = static_cast<Label>(st.out->getInstructions().size() + 2);
		st.emit(Opcode::JUMP_IF_TRUE, { OpBoolReg(balt), OpLabel(afterAlt) });
		// else continue
		// set result true (we'll load True into an expr reg and SAMEQ to produce bool true)
		// patch label: afterAlt:
		st.emit(Opcode::LOAD_IMM, { OpExprReg(st.allocExprReg()), OpImm(Expr::ToExpression("True")) });
		// We won't try to be perfect; just if any alt true we return balt (caller uses it)
		jumpToEndPatches.push_back(afterAlt);
	}

	// Simplify: return the last alt boolean if any; naive but workable for now
	// Better approach: create a dedicated result computation; left as TODO.
	return result;
}
*/

/* Helper: compile normal expressions head[arg1, arg2, ...] */
static void compileNormal(CompilerState& st, std::shared_ptr<MExprNormal> mexpr, Label failLabel)
{
	size_t argsLen = mexpr->length();

	// --- 1. Test argument length ---
	BoolRegIndex bLen = st.allocBoolReg();
	st.emit(Opcode::TEST_LENGTH, { OpBoolReg(bLen), OpExprReg(0), OpImm(argsLen) });
	st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(bLen), OpLabel(failLabel) });

	// --- 2. Check head equality ---
	ExprRegIndex rHead = st.allocExprReg();
	st.emit(Opcode::GET_HEAD, { OpExprReg(rHead), OpExprReg(0) });

	Expr headExpr = mexpr->getHead()->getExpr();
	ExprRegIndex rHeadImm = st.allocExprReg();
	st.emit(Opcode::LOAD_IMM, { OpExprReg(rHeadImm), OpImm(headExpr) });

	BoolRegIndex bHeadEq = st.allocBoolReg();
	st.emit(Opcode::SAMEQ, { OpBoolReg(bHeadEq), OpExprReg(rHead), OpExprReg(rHeadImm) });
	st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(bHeadEq), OpLabel(failLabel) });

	// --- 3. Save r0 before recursive calls ---
	ExprRegIndex rSaved = st.allocExprReg();
	st.emit(Opcode::MOVE, { OpExprReg(rSaved), OpExprReg(0) });

	// --- 4. Compile each argument subpattern ---
	for (mint i = 1; i <= (mint) argsLen; ++i)
	{
		ExprRegIndex rPart = st.allocExprReg();
		st.emit(Opcode::GET_PART, { OpExprReg(rPart), OpExprReg(0), OpImm(i) });

		// Move part into input (register 0)
		st.emit(Opcode::MOVE, { OpExprReg(0), OpExprReg(rPart) });

		auto child = mexpr->part(i);
		compilePatternRec(st, child, failLabel); // propagate failure label

		// Restore input
		st.emit(Opcode::MOVE, { OpExprReg(0), OpExprReg(rSaved) });
	}

	// success: if we reach here, everything matched
}

/* Recursive compiler dispatcher */
static void compilePatternRec(CompilerState& st, std::shared_ptr<MExpr> mexpr, Label failLabel)
{
	// dispatch based on kind
	switch (mexpr->getKind())
	{
		case MExpr::Kind::Literal:
		{
			BoolRegIndex b = compileLiteralMatch(st, mexpr);
			st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(b), OpLabel(failLabel) });
			return;
		}
		case MExpr::Kind::Symbol:
		{
			// TODO: What about symbols that have bindings?
			// For now treat as literal match.
			BoolRegIndex b = compileLiteralMatch(st, mexpr);
			st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(b), OpLabel(failLabel) });
			return;
		}
		case MExpr::Kind::Normal:
			auto mexprNormal = std::static_pointer_cast<MExprNormal>(mexpr);
			if (MExprIsBlank(mexpr))
			{
				BoolRegIndex b = compileBlank(st, mexprNormal);
				st.emit(Opcode::JUMP_IF_FALSE, { OpBoolReg(b), OpLabel(failLabel) });
				return;
			}
			else if (MExprIsPattern(mexpr))
			{
				compilePattern(st, mexprNormal, failLabel);
				return;
			}
			// else if (MExprIsPatternTest(mexpr))
			// {
			// 	return compilePatternTest(st, mexpr);
			// }
			// else if (MExprIsAlternatives(mexpr))
			// {
			// 	return compileAlternatives(st, mexpr);
			// }
			// else if (MExprIsExcept(mexpr))
			// {
			// 	return compileExcept(st, mexprNormal);
			// }
			else
			{
				compileNormal(st, mexprNormal, failLabel);
				return;
			}
	}

	// fallback: immediate failure
	st.emit(Opcode::JUMP, { OpLabel(failLabel) });
}

/* Entry point for compiling a top-level pattern */
std::shared_ptr<PatternBytecode> PatternBytecode::CompilePatternToBytecode(const Expr& patternExpr)
{
	auto pattern = MExpr::construct(patternExpr);

	CompilerState st;

	// Label allocation
	Label entryLabel = st.newLabel();
	Label failLabel = st.newLabel();
	Label successLabel = st.newLabel();

	// Entry block
	st.beginBlock(entryLabel);

	// Compile the pattern
	compilePatternRec(st, pattern, failLabel);

	// If we reach here, pattern matched successfully
	st.emit(Opcode::JUMP, { OpLabel(successLabel) });

	// Failure block
	st.beginBlock(failLabel);
	st.emit(Opcode::DEBUG_PRINT, { OpImm(Expr("Pattern failed")) });
	st.emit(Opcode::LOAD_IMM, { OpBoolReg(0), OpImm(false) }); // load false into %b0
	st.emit(Opcode::HALT, {});

	// Success block
	st.beginBlock(successLabel);
	st.emit(Opcode::DEBUG_PRINT, { OpImm(Expr("Pattern succeeded")) });
	st.emit(Opcode::LOAD_IMM, { OpBoolReg(0), OpImm(true) }); // load true into %b0
	st.emit(Opcode::HALT, {});

	st.out->set_metadata(pattern, st.nextExprReg, st.nextBoolReg, st.lexical);
	return st.out;
}

namespace PatternBytecodeInterface
{
	Expr getBoolRegisterCount(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->getBoolRegisterCount()));
	}
	Expr getExprRegisterCount(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->getExprRegisterCount()));
	}
	Expr getPattern(std::shared_ptr<PatternBytecode> bytecode)
	{
		return MExpr::toExpr(bytecode->getPattern());
	}
	Expr length(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(static_cast<mint>(bytecode->length()));
	}
	Expr toBoxes(Expr objExpr, Expr fmt)
	{
		return Expr::construct("DanielS`PatternMatcher`BackEnd`PatternBytecode`Private`toBoxes", objExpr, fmt);
	}
	Expr toString(std::shared_ptr<PatternBytecode> bytecode)
	{
		return Expr(bytecode->toString());
	}
}; // namespace PatternBytecodeInterface

void PatternBytecode::initializeEmbedMethods(const char* embedName)
{
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getBoolRegisterCount>(
		embedName, "getBoolRegisterCount");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getExprRegisterCount>(
		embedName, "getExprRegisterCount");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::getPattern>(embedName, "getPattern");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::length>(embedName, "length");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::toBoxes>(embedName, "toBoxes");
	RegisterMethod<std::shared_ptr<PatternBytecode>, PatternBytecodeInterface::toString>(embedName, "toString");
}
}; // namespace PatternMatcher