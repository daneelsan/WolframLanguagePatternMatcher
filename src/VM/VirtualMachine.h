#pragma once

#include "ClassSupport.h"
#include "Expr.h"

namespace PatternMatcher
{
extern Expr VirtualMachineExpr();

class VirtualMachine
{
private:
	bool initialized = false;

public:
	VirtualMachine();
	~VirtualMachine();

	void initialize();
	void shutdown();
};

template <>
inline const char* EmbedName<VirtualMachine>()
{
	return "PatternMatcherLibrary`VM`VirtualMachine";
}
}; // namespace PatternMatcher