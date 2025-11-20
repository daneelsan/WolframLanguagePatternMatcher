#include "VM/VirtualMachine.h"

#include "VM/CompilePatternToBytecode.h"
#include "VM/PatternBytecode.h"
#include "VM/Opcode.h"

#include "AST/MExpr.h"

#include "Embeddable.h"
#include "Expr.h"
#include "Logger.h"

#include <memory>
#include <optional>

namespace PatternMatcher
{
/*===========================================================================
VirtualMachine Implementation

This file implements the pattern matching VM. Key design decisions:

1. Register-based (not stack-based) for simpler code generation
2. %e0 always holds "current value being matched"
3. %b0 always holds "final match result"
4. Frames created by BEGIN_BLOCK, popped by END_BLOCK
5. Backtracking restores state but doesn't pop choice points
   (RETRY updates them, TRUST removes them)
6. Trail records bindings only when choice points exist
7. EXPORT_BINDINGS copies final bindings to resultFrame

Execution Flow:
  initialize() -> match(input) -> [execute bytecode] -> HALT -> return result
===========================================================================*/

VirtualMachine::VirtualMachine() = default;
VirtualMachine::~VirtualMachine() = default;

//=============================================================================
// Lifecycle Management
//=============================================================================

void VirtualMachine::initialize(const std::shared_ptr<PatternBytecode>& bytecode_)
{
	if (initialized)
	{
		PM_WARNING("VirtualMachine is already initialized.");
		return;
	}
	initialized = true;
	bytecode = bytecode_;
	reset();
}

void VirtualMachine::shutdown()
{
	if (!initialized)
		return;

	// Clear all state
	bytecode.reset();
	exprRegs.clear();
	boolRegs.clear();
	frames.clear();
	choiceStack.clear();
	trail.clear();
	resultFrame.reset();

	initialized = false;
	halted = false;
}

void VirtualMachine::reset()
{
	// Reset execution state
	pc = 0;
	cycles = 0;
	halted = false;
	backtracking = false;
	unwindingFailure = false;

	// Clear result frame
	resultFrame.reset();

	if (!bytecode)
		return;

	// Allocate registers based on bytecode metadata
	exprRegs.assign(bytecode.value()->getExprRegisterCount(), Expr::ToExpression("Null"));
	boolRegs.assign(bytecode.value()->getBoolRegisterCount(), false);

	// Clear runtime state
	frames.clear();
	choiceStack.clear();
	trail.clear();
}

/*===============================================================
	Opcode Methods
================================================================*/

void VirtualMachine::jump(LabelOp label, bool isFailure)
{
	if (isFailure)
	{
		unwindingFailure = true;
	}
	pc = bytecode.value()->resolveLabel(label.v).value();
	PM_TRACE((isFailure ? "FAIL_JUMP" : "JUMP"), " to L", label.v, " (pc=", pc, ")");
}

void VirtualMachine::saveBindings(Frame& tgtFrame)
{
	PM_ASSERT(!frames.empty(), "saveBindings: No frames available");

	// Copy bindings from current frame to target
	const auto& srcFrame = frames.back();
	for (const auto& [varName, value] : srcFrame.bindings)
	{
		tgtFrame.bindVariable(varName, value);
	}

	PM_TRACE("SAVE_BINDINGS: ", srcFrame.bindings.size(), " bindings copied");
}

//=============================================================================
// Backtracking Support
//=============================================================================

void VirtualMachine::createChoicePoint(size_t nextAlternative)
{
	choiceStack.emplace_back(pc, // Current PC (for debugging)
							 nextAlternative, // Where to jump on FAIL
							 exprRegs, // Copy current expression registers
							 boolRegs, // Copy current boolean registers
							 trail.size(), // Current trail position
							 frames.size() // Current frame depth
	);

	PM_TRACE("CHOICE_POINT created: alternatives at L", nextAlternative, " (stack depth=", choiceStack.size(), ")");
}

bool VirtualMachine::backtrack()
{
	if (choiceStack.empty())
	{
		PM_TRACE("BACKTRACK: No choice points left - FAIL");
		return false;
	}

	// Peek at choice point (don't pop - RETRY/TRUST handle that)
	auto& cp = choiceStack.back();

	// Restore registers from choice point
	exprRegs = cp.savedExprRegs;
	boolRegs = cp.savedBoolRegs;

	// Restore frame stack to saved depth
	while (frames.size() > cp.frameMark)
	{
		frames.pop_back();
	}

	// Unwind trail to undo bindings made after choice point
	unwindTrail(cp.trailMark);

	// Jump to next alternative (resolve label to instruction index)
	auto nextPC = bytecode.value()->resolveLabel(cp.nextAlternative);
	PM_ASSERT(nextPC.has_value(), "Failed to resolve label L", cp.nextAlternative);
	pc = nextPC.value();

	PM_TRACE("BACKTRACK: Restoring choice point, jumping to L", cp.nextAlternative, " (pc=", pc, ")");

	// Set flags
	backtracking = true;
	unwindingFailure = true;

	// Choice point remains on stack!
	// RETRY will update it, TRUST will remove it
	return true;
}

void VirtualMachine::commit()
{
	if (!choiceStack.empty())
	{
		PM_TRACE("COMMIT: Removing ", choiceStack.size(), " choice points");
		choiceStack.clear();
	}
}
void VirtualMachine::trailBind(const std::string& varName, const Expr& value)
{
	// Ensure we have a frame to bind in
	// TODO: Is this a hack?
	if (frames.empty())
	{
		frames.emplace_back();
	}

	auto& currentFrame = frames.back();

	// If variable already bound, record it for potential undo
	// This allows us to restore the old binding on backtrack
	if (currentFrame.hasVariable(varName))
	{
		trail.emplace_back(varName, frames.size() - 1);
		PM_TRACE("TRAIL: Recording existing binding of ", varName, " (trail size=", trail.size(), ")");
	}

	// Bind the variable (overwrites existing binding if any)
	currentFrame.bindVariable(varName, value);

	PM_TRACE("BIND_VAR (trailed): ", varName, " <- ", value.toString());
}

void VirtualMachine::unwindTrail(size_t mark)
{
	if (trail.size() <= mark)
		return; // Nothing to unwind

	PM_TRACE("UNWIND_TRAIL: from ", trail.size(), " to ", mark);

	// Undo bindings in reverse order (LIFO)
	while (trail.size() > mark)
	{
		const auto& entry = trail.back();

		// Remove binding from frame (if frame still exists)
		if (entry.frameIndex < frames.size())
		{
			auto& frame = frames[entry.frameIndex];
			frame.bindings.erase(entry.varName);
			PM_TRACE("  Unbinding: ", entry.varName, " from frame ", entry.frameIndex);
		}

		trail.pop_back();
	}
}

//=============================================================================
// Instruction Execution
//=============================================================================

bool VirtualMachine::step()
{
	// Precondition checks
	if (!initialized || halted || !bytecode)
		return false;

	const auto& instrs = bytecode.value()->getInstructions();
	if (pc >= instrs.size())
	{
		PM_WARNING("PC out of bounds: ", pc, " >= ", instrs.size());
		halted = true;
		return false;
	}

	// Fetch instruction
	const auto& instr = instrs[pc];

	// Update state
	pc += 1;
	cycles += 1;
	backtracking = false;
	unwindingFailure = false;

	// Decode and execute
	switch (instr.opcode)
	{
			//=====================================================================
			// DEBUG
			//=====================================================================

		case Opcode::DEBUG_PRINT:
		{
			if (!instr.ops.empty())
			{
				PM_TRACE("VM_DEBUG_PRINT: ", operandToString(instr.ops[0]));
			}
			break;
		}

			//=====================================================================
			// DATA MOVEMENT
			//=====================================================================

		case Opcode::LOAD_IMM:
		{
			// Immediate can be loaded into expr or bool register
			if (auto dstExprReg = std::get_if<ExprRegOp>(&instr.ops[0]))
			{
				// Load Expr immediate
				auto immExpr = std::get<ImmExpr>(instr.ops[1]);
				exprRegs[dstExprReg->v] = immExpr;
				PM_TRACE("LOAD_IMM %e", dstExprReg->v, " <- ", immExpr.toString());
			}
			else
			{
				// Load boolean immediate (from mint: 0=false, non-zero=true)
				auto dstBoolReg = std::get<BoolRegOp>(instr.ops[0]);
				auto immMint = std::get<ImmMint>(instr.ops[1]);
				bool value = (immMint.v != 0);
				boolRegs[dstBoolReg.v] = value;
				PM_TRACE("LOAD_IMM %b", dstBoolReg.v, " <- ", (value ? "True" : "False"));
			}
			break;
		}

		case Opcode::MOVE:
		{
			auto dst = std::get<ExprRegOp>(instr.ops[0]);
			auto src = std::get<ExprRegOp>(instr.ops[1]);
			exprRegs[dst.v] = exprRegs[src.v];
			PM_TRACE("MOVE %e", dst.v, " <- %e", src.v, " (", exprRegs[src.v], ")");
			break;
		}

			//=====================================================================
			// EXPRESSION INTROSPECTION
			//=====================================================================

		case Opcode::GET_PART:
		{
			auto dst = std::get<ExprRegOp>(instr.ops[0]);
			auto src = std::get<ExprRegOp>(instr.ops[1]);
			auto idx = std::get<ImmMint>(instr.ops[2]);

			exprRegs[dst.v] = exprRegs[src.v].part(idx.v);
			PM_TRACE("GET_PART %e", dst.v, " := part(%e", src.v, ", ", idx.v, ")");
			break;
		}

		case Opcode::GET_LENGTH:
		{
			auto dst = std::get<ExprRegOp>(instr.ops[0]);
			auto src = std::get<ExprRegOp>(instr.ops[1]);

			// Store length as integer expression
			mint len = static_cast<mint>(exprRegs[src.v].length());
			exprRegs[dst.v] = Expr(len);

			PM_TRACE("GET_LENGTH %e", dst.v, " := length(%e", src.v, ") = ", len);
			break;
		}

			//=====================================================================
			// COMPARISON
			//=====================================================================

		case Opcode::APPLY_TEST:
		{
			auto src = std::get<ExprRegOp>(instr.ops[0]);
			auto patternTest = std::get<ImmExpr>(instr.ops[1]);
			auto failLabel = std::get<LabelOp>(instr.ops[2]);

			Expr testRes = Expr::construct(patternTest, exprRegs[src.v]).eval();
			if (testRes)
			{
				PM_TRACE("APPLY_TEST %e", src.v, " with pattern ", patternTest.toString(), " -> SUCCESS");
			}
			else
			{
				PM_TRACE("APPLY_TEST %e", src.v, " with pattern ", patternTest.toString(), " -> FAILURE");
				jump(failLabel, true);
			}
			break;
		}

		case Opcode::SAMEQ:
		{
			auto dstBool = std::get<BoolRegOp>(instr.ops[0]);
			auto lhs = std::get<ExprRegOp>(instr.ops[1]);
			auto rhs = std::get<ExprRegOp>(instr.ops[2]);

			bool result = exprRegs[lhs.v].sameQ(exprRegs[rhs.v]);
			boolRegs[dstBool.v] = result;

			PM_TRACE("SAMEQ %b", dstBool.v, " := (%e", lhs.v, " == %e", rhs.v, ") -> ", (result ? "True" : "False"));
			break;
		}

		case Opcode::MATCH_LENGTH:
		{
			auto src = std::get<ExprRegOp>(instr.ops[0]);
			auto expectedLen = std::get<ImmMint>(instr.ops[1]);
			auto failLabel = std::get<LabelOp>(instr.ops[2]);

			bool matches = (exprRegs[src.v].length() == static_cast<size_t>(expectedLen.v));

			PM_TRACE("MATCH_LENGTH %e", src.v, " == ", expectedLen.v, " -> ", (matches ? "SUCCESS" : "FAILURE"));

			if (!matches)
			{
				jump(failLabel, true);
			}
			break;
		}

		case Opcode::MATCH_HEAD:
		{
			auto src = std::get<ExprRegOp>(instr.ops[0]);
			auto expected = std::get<ImmExpr>(instr.ops[1]);
			auto failLabel = std::get<LabelOp>(instr.ops[2]);

			bool matches = exprRegs[src.v].head().sameQ(expected);

			PM_TRACE("MATCH_HEAD %e", src.v, " == ", expected.toString(), " -> ", (matches ? "SUCCESS" : "FAILURE"));

			if (!matches)
			{
				jump(failLabel, true);
			}
			break;
		}

		case Opcode::MATCH_LITERAL:
		{
			auto src = std::get<ExprRegOp>(instr.ops[0]);
			auto expected = std::get<ImmExpr>(instr.ops[1]);
			auto failLabel = std::get<LabelOp>(instr.ops[2]);

			bool matches = exprRegs[src.v].sameQ(expected);

			PM_TRACE("MATCH_LITERAL %e", src.v, " == ", expected.toString(), " -> ", (matches ? "SUCCESS" : "FAILURE"));

			if (!matches)
			{
				jump(failLabel, true);
			}
			break;
		}

			//=====================================================================
			// SEQUENCE MATCHING
			//=====================================================================

		case Opcode::MATCH_MIN_LENGTH:
		{
			auto src = std::get<ExprRegOp>(instr.ops[0]);
			auto minLen = std::get<ImmMint>(instr.ops[1]);
			auto failLabel = std::get<LabelOp>(instr.ops[2]);

			size_t actualLen = exprRegs[src.v].length();
			bool matches = (actualLen >= static_cast<size_t>(minLen.v));

			PM_TRACE("MATCH_MIN_LENGTH %e", src.v, " >= ", minLen.v, " (actual=", actualLen, ") -> ",
					 (matches ? "SUCCESS" : "FAILURE"));

			if (!matches)
			{
				jump(failLabel, true);
			}
			break;
		}

		case Opcode::MATCH_SEQ_HEADS:
		{
			auto src = std::get<ExprRegOp>(instr.ops[0]);
			auto startIdx = std::get<ImmMint>(instr.ops[1]);
			auto endReg = std::get<ExprRegOp>(instr.ops[2]);
			auto expectedHead = std::get<ImmExpr>(instr.ops[3]);
			auto failLabel = std::get<LabelOp>(instr.ops[4]);

			// End index is in a register (as an integer stored in Expr)
			mint actualEnd = exprRegs[endReg.v].as<mint>().value();
			mint srcLen = static_cast<mint>(exprRegs[src.v].length());

			// Early validation: check bounds once upfront
			if (actualEnd < startIdx.v || startIdx.v < 1 || actualEnd > srcLen)
			{
				PM_TRACE("MATCH_SEQ_HEADS %e", src.v, "[", startIdx.v, "..", actualEnd, "] - INVALID RANGE");
				jump(failLabel, true);
				break;
			}

			// Check each part's head in range [startIdx, actualEnd]
			const Expr& srcExpr = exprRegs[src.v];
			for (mint i = startIdx.v; i <= actualEnd; ++i)
			{
				if (!srcExpr.part(static_cast<size_t>(i)).head().sameQ(expectedHead))
				{
					PM_TRACE("MATCH_SEQ_HEADS %e", src.v, "[", startIdx.v, "..", actualEnd, "] == ", expectedHead.toString(),
							 " -> FAILURE");
					jump(failLabel, true);
					break;
				}
			}

			PM_TRACE("MATCH_SEQ_HEADS %e", src.v, "[", startIdx.v, "..", actualEnd, "] == ", expectedHead.toString(),
					 " -> SUCCESS");
			break;
		}

		case Opcode::MAKE_SEQUENCE:
		{
			auto dst = std::get<ExprRegOp>(instr.ops[0]);
			auto src = std::get<ExprRegOp>(instr.ops[1]);
			auto startIdx = std::get<ImmMint>(instr.ops[2]);

			mint srcLength = static_cast<mint>(exprRegs[src.v].length());

			// Fourth operand can be either ImmMint or ExprRegOp (for dynamic length)
			mint actualEnd;
			if (auto* immEnd = std::get_if<ImmMint>(&instr.ops[3]))
			{
				actualEnd = immEnd->v;
				// Support negative indices: -1 = last, -2 = second-to-last, etc.
				if (actualEnd < 0)
				{
					actualEnd = srcLength + actualEnd + 1;
				}
			}
			else if (auto* regEnd = std::get_if<ExprRegOp>(&instr.ops[3]))
			{
				// End index is in a register (as an integer stored in Expr)
				mint endVal = exprRegs[regEnd->v].as<mint>().value();
				actualEnd = endVal < 0 ? srcLength + endVal + 1 : endVal;
			}
			else
			{
				PM_ERROR("MAKE_SEQUENCE: fourth operand must be ImmMint or ExprRegOp");
				return false;
			}

			// Validate indices (compiler should ensure validity, but check in debug mode)
			PM_ASSERT(startIdx.v >= 1 && startIdx.v <= srcLength + 1, "MAKE_SEQUENCE: invalid start index");
			PM_ASSERT(actualEnd >= 0 && actualEnd <= srcLength, "MAKE_SEQUENCE: invalid end index");

			// Handle empty sequence (startIdx > actualEnd for nullable sequences)
			if (startIdx.v > actualEnd)
			{
				// Create empty Sequence[]
				exprRegs[dst.v] = Expr::createNormal(0, "System`Sequence");
				PM_TRACE("MAKE_SEQUENCE %e", dst.v, " := Sequence[] (empty)");
				break;
			}

			// Extract subsequence and wrap in Sequence
			size_t numParts = static_cast<size_t>(actualEnd - startIdx.v + 1);
			std::vector<Expr> parts;
			parts.reserve(numParts);

			for (mint i = startIdx.v; i <= actualEnd; ++i)
			{
				parts.push_back(exprRegs[src.v].part(static_cast<size_t>(i)));
			}

			// Create Sequence[part1, part2, ...]
			Expr seqExpr = Expr::createNormal(parts.size(), "System`Sequence");
			for (size_t i = 0; i < parts.size(); ++i)
			{
				seqExpr.setPart(i + 1, parts[i]);
			}

			exprRegs[dst.v] = seqExpr;

			PM_TRACE("MAKE_SEQUENCE %e", dst.v, " := Sequence[%e", src.v, "[[", startIdx.v, "..", actualEnd, "]]]");
			break;
		}

		case Opcode::SPLIT_SEQ:
		{
			auto src = std::get<ExprRegOp>(instr.ops[0]);
			auto splitPos = std::get<ImmMint>(instr.ops[1]);
			auto minRest = std::get<ImmMint>(instr.ops[2]);
			auto nextLabel = std::get<LabelOp>(instr.ops[3]);
			auto failLabel = std::get<LabelOp>(instr.ops[4]);

			// This creates a choice point for sequence splitting
			// On backtrack, we'll try the next split position

			// Calculate if this split is valid
			mint totalLen = static_cast<mint>(exprRegs[src.v].length());
			mint seqLen = splitPos.v;
			mint remaining = totalLen - seqLen;

			if (remaining < minRest.v || seqLen < 1)
			{
				// Invalid split, jump to fail
				PM_TRACE("SPLIT_SEQ invalid: splitPos=", splitPos.v, " minRest=", minRest.v, " totalLen=", totalLen);
				jump(failLabel, true);
			}
			else
			{
				// Valid split - create choice point
				// On backtrack, decrement splitPos and retry
				PM_TRACE("SPLIT_SEQ creating choice point: splitPos=", splitPos.v, " nextLabel=", nextLabel.v);

				// TODO: Implement proper choice point with split position tracking
				// For now, this is a placeholder that doesn't actually create backtracking state
				PM_WARNING("SPLIT_SEQ choice point creation not fully implemented");

				// Continue with current split
			}
			break;
		}

			//=====================================================================
			// CONTROL FLOW
			//=====================================================================

		case Opcode::JUMP:
		{
			auto label = std::get<LabelOp>(instr.ops[0]);
			jump(label, false);
			break;
		}

		case Opcode::BRANCH_FALSE:
		{
			auto condReg = std::get<BoolRegOp>(instr.ops[0]);
			auto label = std::get<LabelOp>(instr.ops[1]);

			if (!boolRegs[condReg.v])
			{
				auto targetPC = bytecode.value()->resolveLabel(label.v);
				PM_ASSERT(targetPC.has_value(), "Failed to resolve label L", label.v);
				pc = targetPC.value();
				PM_TRACE("BRANCH_FALSE %b", condReg.v, " to L", label.v, " (pc=", pc, ")");
			}
			else
			{
				PM_TRACE("BRANCH_FALSE %b", condReg.v, " not taken (value is true)");
			}
			break;
		}

		case Opcode::HALT:
		{
			halted = true;
			PM_TRACE("HALT: Stopping execution");
			break;
		}

			//=====================================================================
			// VARIABLE BINDING
			//=====================================================================

		case Opcode::BIND_VAR:
		{
			auto varName = std::get<Ident>(instr.ops[0]);
			auto reg = std::get<ExprRegOp>(instr.ops[1]);

			// Use trail if choice points exist (for backtracking)
			// Otherwise bind directly (optimization)
			if (hasChoicePoints())
			{
				trailBind(varName, exprRegs[reg.v]);
			}
			else
			{
				PM_ASSERT(!frames.empty(), "BIND_VAR: No active frame");
				frames.back().bindVariable(varName, exprRegs[reg.v]);
				PM_TRACE("BIND_VAR (no trail): ", varName, " <- %e", reg.v, " (", exprRegs[reg.v].toString(), ")");
			}
			break;
		}

			//=====================================================================
			// SCOPE MANAGEMENT
			//=====================================================================

		case Opcode::BEGIN_BLOCK:
		{
			auto label = std::get<LabelOp>(instr.ops[0]);
			frames.emplace_back();
			PM_TRACE("BEGIN_BLOCK L", label.v, " (frame depth=", frames.size(), ")");
			break;
		}

		case Opcode::END_BLOCK:
		{
			auto label = std::get<LabelOp>(instr.ops[0]);
			PM_ASSERT(!frames.empty(), "END_BLOCK L", label.v, " with no matching BEGIN_BLOCK");

			// On success path (not unwinding failure), merge bindings upward
			if (frames.size() > 1 && !unwindingFailure)
			{
				auto& parentFrame = frames[frames.size() - 2];
				saveBindings(parentFrame);
				PM_TRACE("END_BLOCK L", label.v, ": Merged bindings to parent");
			}

			// Pop the frame
			frames.pop_back();
			PM_TRACE("END_BLOCK L", label.v, " (popped, depth=", frames.size(), ")");
			break;
		}

		case Opcode::EXPORT_BINDINGS:
		{
			PM_ASSERT(!frames.empty(), "EXPORT_BINDINGS with no active frame");
			saveBindings(resultFrame);
			PM_TRACE("EXPORT_BINDINGS: Saved ", resultFrame.bindings.size(), " bindings to result");
			break;
		}

			//=====================================================================
			// BACKTRACKING
			//=====================================================================

		case Opcode::TRY:
		{
			auto nextAlt = std::get<LabelOp>(instr.ops[0]);
			createChoicePoint(nextAlt.v);
			PM_TRACE("TRY: Choice point created for L", nextAlt.v);
			break;
		}

		case Opcode::RETRY:
		{
			auto nextAlt = std::get<LabelOp>(instr.ops[0]);

			if (!choiceStack.empty())
			{
				choiceStack.back().nextAlternative = nextAlt.v;
				PM_TRACE("RETRY: Updated choice point to L", nextAlt.v);
			}
			else
			{
				PM_WARNING("RETRY with no choice point on stack");
			}
			break;
		}

		case Opcode::TRUST:
		{
			if (!choiceStack.empty())
			{
				choiceStack.pop_back();
				PM_TRACE("TRUST: Removed choice point (last alternative)");
			}
			else
			{
				PM_WARNING("TRUST with no choice point on stack");
			}
			break;
		}

		case Opcode::FAIL:
		{
			PM_TRACE("FAIL: Forcing backtrack");

			if (!backtrack())
			{
				// No choice points left - permanent failure
				halted = true;
				boolRegs[0] = false;
				PM_TRACE("FAIL: No choice points, halting with failure");
				return false;
			}
			break;
		}

		case Opcode::CUT:
		{
			commit();
			PM_TRACE("CUT: All choice points removed");
			break;
		}

			//=====================================================================
			// UNKNOWN OPCODE
			//=====================================================================

		default:
		{
			PM_ERROR("Unknown or unimplemented opcode: ", static_cast<int>(instr.opcode));
			PM_ASSERT(false, "Unimplemented opcode");
			halted = true;
			return false;
		}
	}
	return !halted;
}

//=============================================================================
// High-Level Execution
//=============================================================================

bool VirtualMachine::match(Expr input)
{
	if (!initialized || !bytecode)
	{
		PM_ERROR("match() called on uninitialized VM");
		return false;
	}

	// Reset state and load input
	reset();
	exprRegs[0] = input; // Convention: %e0 holds input

	// Execute until HALT or error
	while (!halted)
	{
		if (!step())
			break;
	}

	// Convention: %b0 holds final match result
	return currentBoolResult();
}

bool VirtualMachine::currentBoolResult()
{
	if (boolRegs.empty())
		return false;
	return boolRegs[0];
}

//=============================================================================
// Embedded Object Interface (Wolfram Language)
//=============================================================================

namespace MethodInterface
{
	Expr compilePattern(VirtualMachine* vm, Expr expr)
	{
		auto bytecode = CompilePatternToBytecode(expr);
		return EmbedObject(bytecode);
	}
	Expr getBytecode(VirtualMachine* vm)
	{
		if (auto bytecodeOpt = vm->getBytecode())
		{
			return EmbedObject(bytecodeOpt.value());
		}
		return Expr::ToExpression("None");
	}
	Expr getCycles(VirtualMachine* vm)
	{
		return Expr(static_cast<mint>(vm->getCycles()));
	}
	Expr getPC(VirtualMachine* vm)
	{
		return Expr(static_cast<mint>(vm->getPC()));
	}
	Expr getResultBindings(VirtualMachine* vm)
	{
		const auto& bindings = vm->getResultBindings();
		Expr bindingsExpr = Expr::createNormal(static_cast<mint>(bindings.size()), "Association");
		mint i = 1;
		for (const auto& [varName, value] : bindings)
		{
			bindingsExpr.setPart(i++, Expr::construct("Rule", Expr(varName.c_str()), value));
		}
		return bindingsExpr;
	}
	Expr initialize(VirtualMachine* vm, Expr bytecodeExpr)
	{
		auto bytecodeOpt = UnembedObject<std::shared_ptr<PatternBytecode>>(bytecodeExpr);
		if (!bytecodeOpt)
		{
			return Expr::throwError("Invalid PatternBytecode object", bytecodeExpr);
		}
		auto bytecode = bytecodeOpt.value();
		vm->initialize(bytecode);
		return Expr::ToExpression("Null");
	}
	Expr isHalted(VirtualMachine* vm)
	{
		return toExpr(vm->isHalted());
	}
	Expr isInitialized(VirtualMachine* vm)
	{
		return toExpr(vm->isInitialized());
	}
	Expr match(VirtualMachine* vm, Expr input)
	{
		bool res = vm->match(input);
		return toExpr(res);
	}
	Expr reset(VirtualMachine* vm)
	{
		vm->reset();
		return Expr::ToExpression("Null");
	}
	Expr shutdown(VirtualMachine* vm)
	{
		vm->shutdown();
		return Expr::ToExpression("Null");
	}
	Expr step(VirtualMachine* vm)
	{
		bool res = vm->step();
		return toExpr(res);
	}
	Expr toBoxes(Expr vmExpr, Expr fmt)
	{
		return Expr::construct("DanielS`PatternMatcher`BackEnd`VirtualMachine`Private`toBoxes", vmExpr, fmt);
	}
	Expr toString(VirtualMachine* vm)
	{
		return Expr("PatternMatcherLibrary`VM`VirtualMachine[...]");
	}
} // namespace MethodInterface

void VirtualMachine::initializeEmbedMethods(const char* embedName)
{
	RegisterMethod<VirtualMachine*, MethodInterface::compilePattern>(embedName, "compilePattern");
	RegisterMethod<VirtualMachine*, MethodInterface::getCycles>(embedName, "getCycles");
	RegisterMethod<VirtualMachine*, MethodInterface::getBytecode>(embedName, "getBytecode");
	RegisterMethod<VirtualMachine*, MethodInterface::getPC>(embedName, "getPC");
	RegisterMethod<VirtualMachine*, MethodInterface::getResultBindings>(embedName, "getResultBindings");
	RegisterMethod<VirtualMachine*, MethodInterface::initialize>(embedName, "initialize");
	RegisterMethod<VirtualMachine*, MethodInterface::isHalted>(embedName, "isHalted");
	RegisterMethod<VirtualMachine*, MethodInterface::isInitialized>(embedName, "isInitialized");
	RegisterMethod<VirtualMachine*, MethodInterface::match>(embedName, "match");
	RegisterMethod<VirtualMachine*, MethodInterface::reset>(embedName, "reset");
	RegisterMethod<VirtualMachine*, MethodInterface::step>(embedName, "step");
	RegisterMethod<VirtualMachine*, MethodInterface::toBoxes>(embedName, "toBoxes");
	RegisterMethod<VirtualMachine*, MethodInterface::toString>(embedName, "toString");
}

Expr VirtualMachineExpr()
{
	VirtualMachine* env = new VirtualMachine();
	return EmbedObject(env);
}
}; // namespace PatternMatcher
