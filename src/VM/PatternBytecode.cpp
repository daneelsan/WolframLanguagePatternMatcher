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
#include <vector>

namespace PatternMatcher
{
void PatternBytecode::push_instr(Opcode op, std::initializer_list<Operand> ops_)
{
	_instrs.push_back(Instruction { op, std::vector<Operand>(ops_) });
}

std::string PatternBytecode::toString() const
{
	std::ostringstream out;
	for (size_t i = 0; i < _instrs.size(); ++i)
	{
		out << i << ": " << opcodeName(_instrs[i].opcode);
		if (!_instrs[i].ops.empty())
		{
			out << " ";
			for (size_t j = 0; j < _instrs[i].ops.size(); ++j)
			{
				out << operandToString(_instrs[i].ops[j]);
				if (j + 1 < _instrs[i].ops.size())
					out << ", ";
			}
		}
		out << "\n";
	}
	out << "Expr registers: " << exprRegisterCount << ", Bool registers: " << boolRegisterCount << "\n";
	if (!lexicalMap.empty())
	{
		out << "lexicalMap:\n";
		for (auto& p : lexicalMap)
			out << "  " << p.first << " -> r" << p.second << "\n";
	}
	return out.str();
}

/*===========================================================================
	Pattern Compilation
===========================================================================*/

/// Simple register allocator + compiler state
struct CompilerState
{
	std::shared_ptr<PatternBytecode> out = std::make_shared<PatternBytecode>();
	ExprRegIndex nextExprReg = 1; // reserve r0 for input expression
	BoolRegIndex nextBoolReg = 0;
	std::unordered_map<std::string, ExprRegIndex> lexical; // lexical var -> reg

	ExprRegIndex allocExprReg() { return nextExprReg++; }
	BoolRegIndex allocBoolReg() { return nextBoolReg++; }

	// convenience append instruction
	void emit(Opcode op, std::initializer_list<Operand> ops = {}) { out->push_instr(op, ops); }
};

/*
 * compilePattern returns a boolean register index that contains whether the
 * pattern matched when executed with the input expression in r0.
 *
 * Convention:
 *   - Input expression to match is in r0.
 *   - compilePattern will allocate registers as needed.
 */
static BoolRegIndex compilePatternRec(CompilerState& st, std::shared_ptr<MExpr> mexpr);

/* Helper: compile literal match: produce bool reg */
static BoolRegIndex compileLiteralMatch(CompilerState& st, std::shared_ptr<MExpr> mexpr)
{
	// allocate an expr reg to hold the literal immediate
	ExprRegIndex rLit = st.allocExprReg();
	st.emit(Opcode::LOAD_IMM, { OpExprReg(rLit), OpImm(mexpr->getExpr()) });
	// allocate bool reg
	BoolRegIndex b = st.allocBoolReg();
	// SAMEQ b, %e0, rLit
	st.emit(Opcode::SAMEQ, { OpBoolReg(b), OpExprReg(0), OpExprReg(rLit) });
	return b;
}

/* Helper: compile Blank (with optional head) */
static BoolRegIndex compileBlank(CompilerState& st, std::shared_ptr<MExprNormal> mexpr)
{
	// Blank[] matches any single expression -> always true.
	// If Blank[f], then check head equality
	if (mexpr->length() == 0)
	{
		// always true
		BoolRegIndex b = st.allocBoolReg();
		// For simplicity set b := true by creating an expression True and comparing True to True
		// TODO: Add a LOAD_TRUE opcode to simplify this
		ExprRegIndex rT = st.allocExprReg();
		st.emit(Opcode::LOAD_IMM, { OpExprReg(rT), OpImm(Expr::ToExpression("True")) });
		st.emit(Opcode::SAMEQ, { OpBoolReg(b), OpExprReg(rT), OpExprReg(rT) });
		return b;
	}
	else
	{
		// mexpr has a head requirement (e.g., Blank[f])
		// Get head of input into rH, compare to head immediate
		ExprRegIndex rH = st.allocExprReg();
		st.emit(Opcode::GET_HEAD, { OpExprReg(rH), OpExprReg(0) });
		// head immediate is the mexpr->part(1) expression
		Expr headExpr = mexpr->part(1)->getExpr();
		ExprRegIndex rHeadImm = st.allocExprReg();
		st.emit(Opcode::LOAD_IMM, { OpExprReg(rHeadImm), OpImm(headExpr) });
		BoolRegIndex b = st.allocBoolReg();
		st.emit(Opcode::SAMEQ, { OpBoolReg(b), OpExprReg(rH), OpExprReg(rHeadImm) });
		return b;
	}
}

/* Helper: compile Pattern[sym, patt] */
static BoolRegIndex compilePattern(CompilerState& st, std::shared_ptr<MExprNormal> mexpr)
{
	// parts: part(1) is symbol (lexical name), part(2) is subpattern
	auto parts = mexpr->getChildren();
	if (parts.size() < 2)
	{
		// malformed; fail
		BoolRegIndex bf = st.allocBoolReg();
		// set bf := false by comparing a False symbol to True symbol
		ExprRegIndex rF = st.allocExprReg();
		st.emit(Opcode::LOAD_IMM, { OpExprReg(rF), OpImm(Expr::ToExpression("False")) });
		ExprRegIndex rT = st.allocExprReg();
		st.emit(Opcode::LOAD_IMM, { OpExprReg(rT), OpImm(Expr::ToExpression("True")) });
		st.emit(Opcode::SAMEQ, { OpBoolReg(bf), OpExprReg(rF), OpExprReg(rT) });
		return bf;
	}

	// get lexical name: first child should be symbol MExprSymbol
	auto symM = parts[0];
	std::string lexName = symM->getExpr().toString(); // fallback; MExprSymbol ideally has getName()
	// If this lexical name has been seen before, check equality with stored var. Otherwise bind.
	auto it = st.lexical.find(lexName);
	if (it != st.lexical.end())
	{
		// repeated variable: compare current expression (r0) to stored reg
		ExprRegIndex storedReg = it->second;
		BoolRegIndex b = st.allocBoolReg();
		st.emit(Opcode::SAMEQ, { OpBoolReg(b), OpExprReg(storedReg), OpExprReg(0) });
		return b;
	}
	else
	{
		// first occurrence: bind var -> r0, record lexical mapping and then compile subpattern
		ExprRegIndex bindReg = st.allocExprReg();
		// Move r0 into bindReg (or treat binding as alias)
		st.emit(Opcode::MOVE, { OpExprReg(bindReg), OpExprReg(0) });
		st.lexical[lexName] = bindReg;
		// emit BIND_VAR as explicit bookkeeping as well
		st.emit(Opcode::BIND_VAR, { OpIdent(lexName), OpExprReg(bindReg) });

		// compile the subpattern: but the subpattern expects input in r0; it will compare directly to r0 or use registers.
		auto subp = parts[1];
		BoolRegIndex bsub = compilePatternRec(st, subp);
		return bsub;
	}
}

/* Helper: compile PatternTest[patt, test] */
static BoolRegIndex compilePatternTest(CompilerState& st, std::shared_ptr<MExprNormal> mexpr)
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
}

/* Helper: compile Except[notPatt] or Except[notPatt, patt] */
static BoolRegIndex compileExcept(CompilerState& st, std::shared_ptr<MExprNormal> mexpr)
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
}

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
static BoolRegIndex compileNormal(CompilerState& st, std::shared_ptr<MExprNormal> mexpr)
{
	// compile as:
	// TEST_LENGTH input == argsLen
	// GET_HEAD -> compare to head immediate
	// for each arg i: GET_PART i into reg, then recursively compile the subpattern for that part with input replaced by reg?

	size_t argsLen = mexpr->length();
	// Test length
	BoolRegIndex bLen = st.allocBoolReg();
	st.emit(Opcode::TEST_LENGTH, { OpBoolReg(bLen), OpExprReg(0), OpImm(argsLen) });

	// Check head equality
	Expr headExpr = mexpr->getHead()->getExpr();
	ExprRegIndex rHead = st.allocExprReg();
	st.emit(Opcode::GET_HEAD, { OpExprReg(rHead), OpExprReg(0) });
	ExprRegIndex rHeadImm = st.allocExprReg();
	st.emit(Opcode::LOAD_IMM, { OpExprReg(rHeadImm), OpImm(headExpr) });
	BoolRegIndex bHeadEq = st.allocBoolReg();
	st.emit(Opcode::SAMEQ, { OpBoolReg(bHeadEq), OpExprReg(rHead), OpExprReg(rHeadImm) });

	// short-circuit: if head != then fail
	// compile arguments: for each arg i, get part i into rArg and compile subpattern
	// we'll naively compile each argument by temporarily moving it into r0 and compiling subpattern expecting r0 as
	// input. To do that we need to MOVE r0 <- partVal and call compilePatternRec. But that modifies r0; we need to
	// restore r0. Save r0 to a temp register
	ExprRegIndex rSaved = st.allocExprReg();
	st.emit(Opcode::MOVE, { OpExprReg(rSaved), OpExprReg(0) });
	BoolRegIndex finalBool = bHeadEq; // we'll combine with args by AND (not implemented as op), so we will check args
									  // sequentially.

	for (mint i = 1; i <= (mint) argsLen; ++i)
	{
		ExprRegIndex rPart = st.allocExprReg();
		st.emit(Opcode::GET_PART, { OpExprReg(rPart), OpExprReg(0), OpImm(Expr(i)) }); // we used integer as Expr(i) - hack
		// move rPart into r0 for subpattern
		st.emit(Opcode::MOVE, { OpExprReg(0), OpExprReg(rPart) });
		// compile subpattern
		// get i-th child
		auto child = mexpr->part(i);
		BoolRegIndex bchild = compilePatternRec(st, child);
		// combine finalBool := finalBool AND bchild
		// We don't have AND; safe fallback: if finalBool false, short-circuit; else result is bchild
		// We'll just set finalBool := bchild if headEq true else false; (imprecise but works as initial pass)
		finalBool = bchild;
		// restore r0
		st.emit(Opcode::MOVE, { OpExprReg(0), OpExprReg(rSaved) });
	}

	// If no args (argsLen==0) finalBool already bHeadEq
	return finalBool;
}

/* Recursive compiler dispatcher */
static BoolRegIndex compilePatternRec(CompilerState& st, std::shared_ptr<MExpr> mexpr)
{
	// dispatch based on kind
	switch (mexpr->getKind())
	{
		case MExpr::Kind::Literal:
		{
			return compileLiteralMatch(st, mexpr);
		}
		break;
		case MExpr::Kind::Symbol:
			// A bare symbol as pattern behaves like literal; compare head or treat as literal?
			// We'll treat symbol in pattern position as literal: same as literal match for the symbol expression.
			return compileLiteralMatch(st, mexpr);
		case MExpr::Kind::Normal:
			auto mexprNormal = std::static_pointer_cast<MExprNormal>(mexpr);
			if (MExprIsBlank(mexpr))
			{
				return compileBlank(st, mexprNormal);
			}
			// else if (MExprIsPattern(mexpr))
			// {
			// 	return compilePattern(st, mexpr);
			// }
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
			// else
			// {
			// 	return compileNormal(st, mexprNormal);
			// }
	}

	// fallback: fail
	BoolRegIndex bf = st.allocBoolReg();
	ExprRegIndex rF = st.allocExprReg();
	ExprRegIndex rT = st.allocExprReg();
	st.emit(Opcode::LOAD_IMM, { OpExprReg(rF), OpImm(Expr::ToExpression("False")) });
	st.emit(Opcode::LOAD_IMM, { OpExprReg(rT), OpImm(Expr::ToExpression("True")) });
	st.emit(Opcode::SAMEQ, { OpBoolReg(bf), OpExprReg(rF), OpExprReg(rT) });
	return bf;
}

/* Entry point */
std::shared_ptr<PatternBytecode> PatternBytecode::CompilePatternToBytecode(const Expr& pattern)
{
	// Build MExpr from pattern for easier structural analysis
	auto mexpr = MExpr::construct(pattern);

	CompilerState st;
	// reserve r0 as input (convention)
	st.nextExprReg = 1; // r0 is reserved; allocExprReg returns 1 first -> r1 etc.
	st.nextBoolReg = 0;

	// top-level compile: returns a boolean register
	BoolRegIndex resultBool = compilePatternRec(st, mexpr);

	// Emit HALT
	st.emit(Opcode::HALT, {});

	// Set metadata
	st.out->set_metadata(mexpr, st.nextExprReg, st.nextBoolReg, st.lexical);

	// optionally emit a final instruction to move boolean result to a known place if needed

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