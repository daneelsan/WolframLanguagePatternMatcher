#include "AST/MExpr.h"

#include "Logger.h"

namespace PatternMatcher
{
std::shared_ptr<MExpr> MExprNormal::getHead() const
{
	return _head;
}

bool MExprNormal::sameQ(std::shared_ptr<MExpr> other) const
{
	if (other->getKind() != Kind::Normal)
		return false;
	auto o = std::static_pointer_cast<MExprNormal>(other);
	return _expr.sameQ(o->_expr); // TODO: deep compare children
}

void MExprNormal::initializeEmbedMethods(const char* embedName)
{
	initializeEmbedMethodsCommon<MExprNormal>(embedName);
}
}; // namespace PatternMatcher