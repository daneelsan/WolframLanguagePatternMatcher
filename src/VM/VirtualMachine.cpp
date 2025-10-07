#include "VM/VirtualMachine.h"

#include "Embeddable.h"
#include "Expr.h"

namespace PatternMatcher
{
Expr VirtualMachineExpr()
{
	VirtualMachine* env = new VirtualMachine();
	return EmbedObject(env);
}
}; // namespace PatternMatcher
