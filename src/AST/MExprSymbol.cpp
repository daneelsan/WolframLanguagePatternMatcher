#include "AST/MExpr.h"

#include "Logger.h"

namespace PatternMatcher
{
bool MExprSymbol::sameQ(std::shared_ptr<MExpr> other) const
{
	if (other->getKind() != Kind::Symbol)
		return false;
	auto o = std::static_pointer_cast<MExprSymbol>(other);
	return _expr.sameQ(o->_expr) && name == o->name && context == o->context;
}

namespace MethodInterface {
    
}; // namespace MethodInterface

void MExprSymbol::initializeEmbedMethods(const char* embedName)
{
	initializeEmbedMethodsCommon<MExprSymbol>(embedName);
}
}; // namespace PatternMatcher