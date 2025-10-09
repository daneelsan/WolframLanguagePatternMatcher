#include "VM/VirtualMachine.h"

#include "AST/MExpr.h"
#include "VM/PatternBytecode.h"

#include "Embeddable.h"
#include "Expr.h"
#include "Logger.h"

#include <memory>

namespace PatternMatcher
{

VirtualMachine::VirtualMachine() = default;
VirtualMachine::~VirtualMachine() = default;

void VirtualMachine::initialize()
{
	// TODO: Add any initialization logic if needed
	if (initialized)
		return;
	initialized = true;
}

void VirtualMachine::shutdown()
{
	// TODO: Add any shutdown logic if needed
	if (!initialized)
		return;
	initialized = false;
}

void VirtualMachine::reset()
{
	pc = 0;
	cycles = 0;
	halted = false;
}

namespace MethodInterface
{
	Expr compilePattern(VirtualMachine* vm, Expr expr)
	{
		auto bytecode = PatternBytecode::CompilePatternToBytecode(expr);
		return EmbedObject(bytecode);
	}
	Expr getPC(VirtualMachine* vm)
	{
		return Expr(static_cast<mint>(vm->getPC()));
	}
	Expr getCycles(VirtualMachine* vm)
	{
		return Expr(static_cast<mint>(vm->getCycles()));
	}
	Expr isHalted(VirtualMachine* vm)
	{
		return toExpr(vm->isHalted());
	}
	Expr reset(VirtualMachine* vm)
	{
		vm->reset();
		return Expr::ToExpression("Null");
	}
	Expr toString(VirtualMachine* vm)
	{
		return Expr("PatternMatcherLibrary`VM`VirtualMachine[...]");
	}
} // namespace MethodInterface

void VirtualMachine::initializeEmbedMethods(const char* embedName)
{
	AddCompilerClassMethod_Export(
		embedName, "compilePattern",
		reinterpret_cast<void*>(&embeddedObjectUnaryMethod<VirtualMachine*, Expr, MethodInterface::compilePattern>));
	AddCompilerClassMethod_Export(
		embedName, "getCycles",
		reinterpret_cast<void*>(&embeddedObjectNullaryMethod<VirtualMachine*, MethodInterface::getCycles>));
	AddCompilerClassMethod_Export(
		embedName, "getPC", reinterpret_cast<void*>(&embeddedObjectNullaryMethod<VirtualMachine*, MethodInterface::getPC>));
	AddCompilerClassMethod_Export(
		embedName, "isHalted",
		reinterpret_cast<void*>(&embeddedObjectNullaryMethod<VirtualMachine*, MethodInterface::isHalted>));
	AddCompilerClassMethod_Export(
		embedName, "reset", reinterpret_cast<void*>(&embeddedObjectNullaryMethod<VirtualMachine*, MethodInterface::reset>));
	AddCompilerClassMethod_Export(
		embedName, "toString",
		reinterpret_cast<void*>(&embeddedObjectNullaryMethod<VirtualMachine*, MethodInterface::toString>));
}

Expr VirtualMachineExpr()
{
	VirtualMachine* env = new VirtualMachine();
	return EmbedObject(env);
}
}; // namespace PatternMatcher
