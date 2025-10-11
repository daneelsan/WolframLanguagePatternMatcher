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
	initialized = false;
}

void VirtualMachine::reset()
{
	pc = 0;
	cycles = 0;
	halted = false;

	if (!bytecode)
	{
		return;
	}
	exprRegs.assign(bytecode.value()->getExprRegisterCount(), Expr::ToExpression("Null")); // TODO: better initial value?
	boolRegs.assign(bytecode.value()->getBoolRegisterCount(), false);
	frames.clear();
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

	switch (instr.opcode)
	{
		case Opcode::HALT:
		{
			halted = true;
			break;
		}
		case Opcode::DEBUG_PRINT:
		{
			if (!instr.ops.empty())
			{
				PM_TRACE("VM_DEBUG_PRINT: ", operandToString(instr.ops[0]));
			}
			break;
		}
		case Opcode::LOAD_IMM:
		{
			// Handle Expr register
			if (auto dstExprReg = std::get_if<ExprRegOp>(&instr.ops[0]))
			{
				if (auto immExpr = std::get_if<ImmExpr>(&instr.ops[1]))
				{
					exprRegs[dstExprReg->v] = *immExpr;
					PM_TRACE("LOAD_IMM %e", dstExprReg->v, " <- ", immExpr->toString());
				}
				else
				{
					PM_ERROR("LOAD_IMM: Expected ImmExpr for expr register");
					halted = true;
					return false;
				}
			}
			else
			{
				// Support loading immediate mint into boolean register
				auto dstBoolReg = std::get_if<BoolRegOp>(&instr.ops[0]);
				auto immMint = std::get_if<ImmMint>(&instr.ops[1]);
				if (dstBoolReg && immMint)
				{
					// Load immediate mint into the boolean register (nonzero -> true)
					bool res = (immMint->v != 0);
					boolRegs[dstBoolReg->v] = res;
					PM_TRACE("LOAD_IMM %e", dstBoolReg->v, " <- ", (res) ? "True" : "False");
				}
				else
				{
					// Invalid operands
					PM_ERROR("LOAD_IMM: Invalid operands");
					halted = true;
					return false;
				}
			}
			break;
		}
		case Opcode::MOVE:
		{
			auto dst = std::get_if<ExprRegOp>(&instr.ops[0]);
			auto src = std::get_if<ExprRegOp>(&instr.ops[1]);
			if (dst && src)
			{
				exprRegs[dst->v] = exprRegs[src->v];
				PM_TRACE("MOVE %e", dst->v, " <- %e", src->v, " (", exprRegs[src->v].toString(), ")");
			}
			else
			{
				PM_ERROR("MOVE: Invalid operands");
				halted = true;
				return false;
			}
			break;
		}
		case Opcode::GET_HEAD:
		{
			auto dstExpr = std::get_if<ExprRegOp>(&instr.ops[0]);
			auto srcExpr = std::get_if<ExprRegOp>(&instr.ops[1]);

			if (dstExpr && srcExpr)
			{
				const auto& expr = exprRegs[srcExpr->v];
				exprRegs[dstExpr->v] = expr.head();
				PM_TRACE("GET_HEAD %e", dstExpr->v, " := head(%e", srcExpr->v, ")");
			}
			else
			{
				PM_ERROR("GET_HEAD: Invalid operands");
				halted = true;
				return false;
			}
			break;
		}

		case Opcode::GET_PART:
		{
			auto dstExpr = std::get_if<ExprRegOp>(&instr.ops[0]);
			auto srcExpr = std::get_if<ExprRegOp>(&instr.ops[1]);
			auto immIdx = std::get_if<ImmMint>(&instr.ops[2]);

			if (dstExpr && srcExpr && immIdx)
			{
				const auto& expr = exprRegs[srcExpr->v];
				size_t index = static_cast<size_t>(immIdx->v);
				if (index >= 1 && index <= expr.length()) // TODO: If compilation was correct, this should always be true
				{
					exprRegs[dstExpr->v] = expr.part(index);
					PM_TRACE("GET_PART %e", dstExpr->v, " := part(", index, ", %e", srcExpr->v, ")");
				}
				else
					PM_ERROR("GET_PART: Index out of bounds");
			}
			else
			{
				PM_ERROR("GET_PART: Invalid operands");
				halted = true;
				return false;
			}
			break;
		}
		case Opcode::TEST_LENGTH:
		{
			auto dstBool = std::get_if<BoolRegOp>(&instr.ops[0]);
			auto srcExpr = std::get_if<ExprRegOp>(&instr.ops[1]);
			auto immLen = std::get_if<ImmMint>(&instr.ops[2]);

			if (dstBool && srcExpr && immLen)
			{
				const auto& expr = exprRegs[srcExpr->v];
				size_t len = expr.length();
				bool result = (len == static_cast<size_t>(immLen->v));
				boolRegs[dstBool->v] = result;
				PM_TRACE("TEST_LENGTH %b", dstBool->v, " := length(%e", srcExpr->v, ") == ", immLen->v, " -> ",
						 (result ? "True" : "False"));
			}
			else
			{
				PM_ERROR("TEST_LENGTH: Invalid operands");
				halted = true;
				return false;
			}
			break;
		}
		case Opcode::SAMEQ:
		{
			auto dstBool = std::get_if<BoolRegOp>(&instr.ops[0]);
			auto lhs = std::get_if<ExprRegOp>(&instr.ops[1]);
			auto rhs = std::get_if<ExprRegOp>(&instr.ops[2]);

			if (dstBool && lhs && rhs)
			{
				// TODO: Careful about evaluating the exprs in sameQ?
				bool result = exprRegs[lhs->v].sameQ(exprRegs[rhs->v]);
				boolRegs[dstBool->v] = result;
				PM_TRACE("SAMEQ %b", dstBool->v, " := (%e", lhs->v, " == %e", rhs->v, ") -> ", (result ? "True" : "False"));
			}
			else
			{
				PM_ERROR("SAMEQ: Invalid operands");
				halted = true;
				return false;
			}
			break;
		}
		case Opcode::MATCH_HEAD:
		{
			auto srcExpr = std::get_if<ExprRegOp>(&instr.ops[0]);
			auto immExpr = std::get_if<ImmExpr>(&instr.ops[1]);
			auto labelOp = std::get_if<LabelOp>(&instr.ops[2]);
			if (srcExpr && immExpr && labelOp)
			{
				const auto& expr = exprRegs[srcExpr->v];
				bool result = expr.head().sameQ(*immExpr);
				PM_TRACE("MATCH_HEAD %e", srcExpr->v, " head == ", immExpr->toString(), " -> ", (result ? "True" : "False"));
				if (!result)
				{
					pc = bytecode.value()->resolveLabel(labelOp->v).value();
					PM_TRACE("  -> JUMP to L", labelOp->v, " (pc=", pc, ")");
				}
			}
			else
			{
				PM_ERROR("MATCH_HEAD: Invalid operands");
				halted = true;
				return false;
			}
			break;
		}
		case Opcode::JUMP:
		{
			if (auto L = std::get_if<LabelOp>(&instr.ops[0]))
			{
				auto targetOpt = bytecode.value()->resolveLabel(L->v);
				// BUG FIX: Check if label resolution succeeded
				if (!targetOpt)
				{
					PM_ERROR("JUMP: Label L", L->v, " not found");
					halted = true;
					return false;
				}
				pc = targetOpt.value();
				PM_TRACE("JUMP to L", L->v, " (pc=", pc, ")");
			}
			else
			{
				PM_ERROR("JUMP: Invalid operand");
				halted = true;
				return false;
			}
			break;
		}
		case Opcode::JUMP_IF_FALSE:
		{
			auto breg = std::get_if<BoolRegOp>(&instr.ops[0]);
			auto L = std::get_if<LabelOp>(&instr.ops[1]);
			if (breg && L)
			{
				// BUG FIX: Add bounds checking
				if (breg->v >= boolRegs.size())
				{
					PM_ERROR("JUMP_IF_FALSE: Bool register index out of bounds");
					halted = true;
					return false;
				}
				if (!boolRegs[breg->v])
				{
					auto targetOpt = bytecode.value()->resolveLabel(L->v);
					// BUG FIX: Check if label resolution succeeded
					if (!targetOpt)
					{
						PM_ERROR("JUMP_IF_FALSE: Label L", L->v, " not found");
						halted = true;
						return false;
					}
					pc = targetOpt.value();
					PM_TRACE("JUMP_IF_FALSE %b", breg->v, " to L", L->v, " (pc=", pc, ")");
				}
				else
				{
					PM_TRACE("JUMP_IF_FALSE %b", breg->v, " not taken (value is true)");
				}
			}
			else
			{
				PM_ERROR("JUMP_IF_FALSE: Invalid operands");
				halted = true;
				return false;
			}
			break;
		}
		case Opcode::BIND_VAR:
		{
			auto ident = std::get_if<Ident>(&instr.ops[0]);
			auto ereg = std::get_if<ExprRegOp>(&instr.ops[1]);
			if (!ident || !ereg)
			{
				PM_ERROR("BIND_VAR: invalid operands");
				halted = true;
				return false;
			}
			if (frames.empty())
			{
				// If no frame exists, create one — safe fallback.
				frames.emplace_back();
			}
			// store a copy of the expression value in the current frame
			frames.back().insert_or_assign(*ident, exprRegs[ereg->v]);
			PM_TRACE("BIND_VAR: ", *ident, " <- %e", ereg->v, "  (", exprRegs[ereg->v].toString(), ")");
			break;
		}
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
