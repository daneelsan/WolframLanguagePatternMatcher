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

VirtualMachine::VirtualMachine() = default;
VirtualMachine::~VirtualMachine() = default;

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
	// TODO: Add any shutdown logic if needed
	if (!initialized)
		return;
	bytecode.reset();
	exprRegs.clear();
	boolRegs.clear();

	choiceStack.clear();
	trail.clear();

	initialized = false;
}

void VirtualMachine::reset()
{
	pc = 0;
	cycles = 0;
	halted = false;
	backtracking = false;

	if (!bytecode)
	{
		return;
	}
	exprRegs.assign(bytecode.value()->getExprRegisterCount(), Expr::ToExpression("Null")); // TODO: better initial value?
	boolRegs.assign(bytecode.value()->getBoolRegisterCount(), false);
	frames.clear();

	choiceStack.clear();
	trail.clear();
}

/*===============================================================
	Backtracking Support
================================================================*/
void VirtualMachine::createChoicePoint(size_t nextAlternative)
{
	choiceStack.emplace_back(pc, // Current PC
							 nextAlternative, // Where to jump on backtrack
							 exprRegs, // Copy current registers
							 boolRegs,
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
		return false; // No more alternatives - pattern fails
	}

	// Pop most recent choice point
	ChoicePoint cp = std::move(choiceStack.back());
	choiceStack.pop_back();

	// Restore VM state
	PM_TRACE("BACKTRACK: Restoring choice point, jumping to L", cp.nextAlternative);
	pc = cp.nextAlternative; // Jump to next alternative
	exprRegs = std::move(cp.savedExprRegs);
	boolRegs = std::move(cp.savedBoolRegs);

	// Unwind trail - undo variable bindings
	unwindTrail(cp.trailMark);

	// Restore frame depth
	while (frames.size() > cp.frameMark)
	{
		frames.pop_back();
	}

	backtracking = true;
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
	if (frames.empty())
	{
		frames.emplace_back();
	}

	auto& currentFrame = frames.back();

	// If variable already bound, record it for undoing
	if (currentFrame.find(varName) != currentFrame.end())
	{
		trail.emplace_back(varName, frames.size() - 1);
	}

	// Bind the variable
	currentFrame.insert_or_assign(varName, value);

	PM_TRACE("TRAIL_BIND: ", varName, " <- ", value.toString(), " (trail size=", trail.size(), ")");
}

void VirtualMachine::unwindTrail(size_t mark)
{
	PM_TRACE("UNWIND_TRAIL: from ", trail.size(), " to ", mark);

	// Undo bindings in reverse order
	while (trail.size() > mark)
	{
		const auto& entry = trail.back();

		if (entry.frameIndex < frames.size())
		{
			auto& frame = frames[entry.frameIndex];
			frame.erase(entry.varName);
			PM_TRACE("  Unbinding: ", entry.varName);
		}

		trail.pop_back();
	}
}

/*===============================================================
	Execute a single instruction
================================================================*/
bool VirtualMachine::step()
{
	if (!initialized || halted || !bytecode)
		return false;

	auto& instrs = bytecode.value()->getInstructions();
	if (pc >= instrs.size())
	{
		halted = true;
		return false;
	}

	const auto& instr = instrs[pc];
	pc += 1;
	cycles += 1;
	backtracking = false;

	switch (instr.opcode)
	{
		case Opcode::DEBUG_PRINT:
		{
			if (!instr.ops.empty())
			{
				PM_TRACE("VM_DEBUG_PRINT: ", operandToString(instr.ops[0]));
			}
			break;
		}

		/*
			Register Operations
		*/
		case Opcode::LOAD_IMM:
		{
			// Fast path: Expr register (most common)
			if (auto dstExprReg = std::get_if<ExprRegOp>(&instr.ops[0]))
			{
				auto immExpr = std::get<ImmExpr>(instr.ops[1]);
				exprRegs[dstExprReg->v] = immExpr;
				PM_TRACE("LOAD_IMM %e", dstExprReg->v, " <- ", immExpr.toString());
			}
			else
			{
				// Bool register path
				auto dstBoolReg = std::get<BoolRegOp>(instr.ops[0]);
				auto immMint = std::get<ImmMint>(instr.ops[1]);
				bool res = immMint.v != 0;
				boolRegs[dstBoolReg.v] = res;
				PM_TRACE("LOAD_IMM %b", dstBoolReg.v, " <- ", res ? "True" : "False");
			}
			// NOTE: consider splitting into two opcodes for clarity/performance?
			break;
		}
		case Opcode::MOVE:
		{
			auto dst = std::get<ExprRegOp>(instr.ops[0]);
			auto src = std::get<ExprRegOp>(instr.ops[1]);
			exprRegs[dst.v] = exprRegs[src.v];
			PM_TRACE("MOVE %e", dst.v, " <- %e", src.v, " (", exprRegs[src.v].toString(), ")");
			break;
		}

		/*
			Introspection Operations
		*/
		case Opcode::GET_HEAD:
		{
			auto dst = std::get<ExprRegOp>(instr.ops[0]);
			auto src = std::get<ExprRegOp>(instr.ops[1]);
			exprRegs[dst.v] = exprRegs[src.v].head();
			PM_TRACE("GET_HEAD %e", dst.v, " := head(%e", src.v, ")");
			break;
		}
		case Opcode::GET_PART:
		{
			auto dst = std::get<ExprRegOp>(instr.ops[0]);
			auto src = std::get<ExprRegOp>(instr.ops[1]);
			auto idx = std::get<ImmMint>(instr.ops[2]);
			exprRegs[dst.v] = exprRegs[src.v].part(static_cast<size_t>(idx.v));
			PM_TRACE("GET_PART %e", dst.v, " := part(", "%e", src.v, ", ", idx.v, ")");
			break;
		}
		case Opcode::TEST_LENGTH:
		{
			auto dstBool = std::get<BoolRegOp>(instr.ops[0]);
			auto srcExpr = std::get<ExprRegOp>(instr.ops[1]);
			auto immLen = std::get<ImmMint>(instr.ops[2]);

			const auto& expr = exprRegs[srcExpr.v];
			size_t len = expr.length();
			bool result = (len == static_cast<size_t>(immLen.v));
			boolRegs[dstBool.v] = result;
			PM_TRACE("TEST_LENGTH %b", dstBool.v, " := length(%e", srcExpr.v, ") == ", immLen.v, " -> ",
					 (result ? "True" : "False"));
			break;
		}
		case Opcode::SAMEQ:
		{
			auto dstBool = std::get<BoolRegOp>(instr.ops[0]);
			auto lhs = std::get<ExprRegOp>(instr.ops[1]);
			auto rhs = std::get<ExprRegOp>(instr.ops[2]);
			// TODO: Careful about evaluating the exprs in sameQ?
			bool result = exprRegs[lhs.v].sameQ(exprRegs[rhs.v]);
			PM_TRACE("SAMEQ %b", dstBool.v, " := (%e", lhs.v, " == %e", rhs.v, ") -> ", (result ? "True" : "False"));
			boolRegs[dstBool.v] = result;
			break;
		}

		/*
			Match Operations
		*/
		case Opcode::MATCH_LENGTH:
		{
			auto src = std::get<ExprRegOp>(instr.ops[0]);
			auto expectedLen = std::get<ImmMint>(instr.ops[1]);
			auto label = std::get<LabelOp>(instr.ops[2]);

			bool res = exprRegs[src.v].length() == static_cast<size_t>(expectedLen.v);
			PM_TRACE("MATCH_LENGTH", " %e", src.v, " == ", expectedLen.v, " -> ", (res ? "SUCCESS" : "FAILURE"));
			if (not res)
			{
				pc = bytecode.value()->resolveLabel(label.v).value();
				PM_TRACE("MATCH_LENGTH failed -> JUMP to L", label.v);
			}
			break;
		}
		case Opcode::MATCH_HEAD:
		{
			auto src = std::get<ExprRegOp>(instr.ops[0]);
			auto expected = std::get<ImmExpr>(instr.ops[1]);
			auto label = std::get<LabelOp>(instr.ops[2]);

			auto expr = exprRegs[src.v];
			bool res = expr.head().sameQ(expected);
			PM_TRACE("MATCH_HEAD", " %e", src.v, " == ", expected.toString(), " -> ", (res ? "SUCCESS" : "FAILURE"));
			if (not res)
			{
				pc = bytecode.value()->resolveLabel(label.v).value();
				PM_TRACE("	MATCH failed -> JUMP to L", label.v);
			}
			break;
		}
		case Opcode::MATCH_LITERAL:
		{
			auto src = std::get<ExprRegOp>(instr.ops[0]);
			auto expected = std::get<ImmExpr>(instr.ops[1]);
			auto label = std::get<LabelOp>(instr.ops[2]);

			auto expr = exprRegs[src.v];
			bool res = expr.sameQ(expected);
			PM_TRACE("MATCH_LITERAL", " %e", src.v, " == ", expected.toString(), " -> ", (res ? "SUCCESS" : "FAILURE"));
			if (not res)
			{
				pc = bytecode.value()->resolveLabel(label.v).value();
				PM_TRACE("	MATCH failed -> JUMP to L", label.v);
			}
			break;
		}

		/*
			Halt Operations
		*/
		case Opcode::HALT:
		{
			halted = true;
			PM_TRACE("HALT: Stopping execution");
			break;
		}
		case Opcode::JUMP:
		{
			auto label = std::get<LabelOp>(instr.ops[0]);
			pc = bytecode.value()->resolveLabel(label.v).value(); // Assume valid in release
			PM_TRACE("JUMP to L", label.v, " (pc=", pc, ")");
			break;
		}
		case Opcode::JUMP_IF_FALSE:
		{
			auto breg = std::get<BoolRegOp>(instr.ops[0]);
			auto label = std::get<LabelOp>(instr.ops[1]);
			if (!boolRegs[breg.v])
			{
				pc = bytecode.value()->resolveLabel(label.v).value();
				PM_TRACE("JUMP_IF_FALSE %b", breg.v, " to L", label.v, " (pc=", pc, ")");
			}
			break;
		}
		/*
			Binding Operations
		*/
		case Opcode::BIND_VAR:
		{
			auto ident = std::get<Ident>(instr.ops[0]);
			auto reg = std::get<ExprRegOp>(instr.ops[1]);

			// Use trailBind instead of direct binding for backtracking support
			if (hasChoicePoints())
			{
				trailBind(ident, exprRegs[reg.v]);
			}
			else
			{
				// No choice points - can bind directly (optimization)
				// NOTE: There should always be at least one frame. Is this an error?
				if (frames.empty())
					frames.emplace_back();
				frames.back().insert_or_assign(ident, exprRegs[reg.v]);
				PM_TRACE("BIND_VAR (no trail): ", ident, " <- %e", reg.v);
			}
			break;
		}
		case Opcode::GET_VAR:
		{
			auto dst = std::get<ExprRegOp>(instr.ops[0]);
			auto ident = std::get<Ident>(instr.ops[1]);
			// Search frames from most recent to oldest
			for (auto it = frames.rbegin(); it != frames.rend(); ++it)
			{
				if (auto found = it->find(ident); found != it->end())
				{
					exprRegs[dst.v] = found->second;
					PM_TRACE("GET_VAR %e", dst.v, " <- ", ident);
					goto next_instruction;
				}
			}
		next_instruction:
			break;
		}

		/*
			Scope Management
		*/
		case Opcode::BEGIN_BLOCK:
		{
			// push a new frame for bindings
			frames.emplace_back();
			// optional: if the opcode passes a label operand, show it
			if (!instr.ops.empty())
			{
				if (auto L = std::get_if<LabelOp>(&instr.ops[0]))
					PM_TRACE("BEGIN_BLOCK L", L->v, " (frame depth=", frames.size(), ")");
				else
					PM_TRACE("BEGIN_BLOCK (frame depth=", frames.size(), ")");
			}
			else
			{
				PM_TRACE("BEGIN_BLOCK (frame depth=", frames.size(), ")");
			}
			break;
		}
		case Opcode::END_BLOCK:
		{
			if (!frames.empty())
			{
				frames.pop_back();
				if (!instr.ops.empty())
				{
					if (auto L = std::get_if<LabelOp>(&instr.ops[0]))
						PM_TRACE("END_BLOCK L", L->v, " (popped, depth=", frames.size(), ")");
					else
						PM_TRACE("END_BLOCK (popped, depth=", frames.size(), ")");
				}
				else
				{
					PM_TRACE("END_BLOCK (popped, depth=", frames.size(), ")");
				}
			}
			else
			{
				// BUG FIX: This is a serious error - stack underflow
				PM_ERROR("END_BLOCK: no frame to pop (stack underflow)");
				if (!instr.ops.empty())
				{
					if (auto L = std::get_if<LabelOp>(&instr.ops[0]))
						PM_ERROR("  at label L", L->v);
				}
				// Continue execution but log the error
			}
			break;
		}

		/*
			Backtracking support
		*/
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
			// Update the top choice point to try next alternative
			if (!choiceStack.empty())
			{
				choiceStack.back().nextAlternative = nextAlt.v;
				PM_TRACE("RETRY: Updated choice point to L", nextAlt.v);
			}
			break;
		}
		case Opcode::TRUST:
		{
			// Last alternative - remove choice point and continue
			if (!choiceStack.empty())
			{
				choiceStack.pop_back();
				PM_TRACE("TRUST: Removed choice point (last alternative)");
			}
			break;
		}
		case Opcode::FAIL:
		{
			PM_TRACE("FAIL: Forcing backtrack");
			if (!backtrack())
			{
				halted = true;
				boolRegs[0] = false; // Pattern failed
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
		case Opcode::TRAIL_BIND:
		{
			auto ident = std::get<Ident>(instr.ops[0]);
			auto reg = std::get<ExprRegOp>(instr.ops[1]);
			trailBind(ident, exprRegs[reg.v]);
			break;
		}
		case Opcode::SAVE_STATE:
		{
			// Explicit state save (for complex patterns)
			if (!choiceStack.empty())
			{
				auto& cp = choiceStack.back();
				cp.savedExprRegs = exprRegs;
				cp.savedBoolRegs = boolRegs;
				PM_TRACE("SAVE_STATE: Registers saved to choice point");
			}
			break;
		}
		case Opcode::RESTORE_STATE:
		{
			// Explicit state restore
			if (!choiceStack.empty())
			{
				const auto& cp = choiceStack.back();
				exprRegs = cp.savedExprRegs;
				boolRegs = cp.savedBoolRegs;
				PM_TRACE("RESTORE_STATE: Registers restored from choice point");
			}
			break;
		}

		default:
		{
			PM_DEBUG("Opcode not implemented yet: ", static_cast<int>(instr.opcode));
			halted = true;
			return false;
		}
	}
	return !halted;
}

/*===============================================================
	Run full pattern match
================================================================*/
bool VirtualMachine::match(Expr input)
{
	if (!initialized || !bytecode)
		return false;

	reset();
	exprRegs[0] = input;

	while (!halted)
	{
		step();
	}

	// convention: %b0 holds the result (True=match, False=no match)
	return currentBoolResult();
}

/*===============================================================
	Helper to get match result
================================================================*/
bool VirtualMachine::currentBoolResult()
{
	if (boolRegs.empty())
		return false;
	return boolRegs[0];
}

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
