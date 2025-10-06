#include "AST/MExpr.h"

#include "Logger.h"

namespace PatternMatcher
{
std::shared_ptr<MExpr> MExprLiteral::create(Expr e)
{
	return std::make_shared<MExprLiteral>(e);
}

std::shared_ptr<MExpr> MExprLiteral::getHead() const
{
	return MExpr::construct(_expr.head());
}

bool MExprLiteral::sameQ(std::shared_ptr<MExpr> other) const
{
	if (other->getKind() != Kind::Literal)
		return false;
	auto o = std::static_pointer_cast<MExprLiteral>(other);
	return _expr.sameQ(o->_expr);
}

void MExprLiteral::initializeEmbedMethods(const char* embedName)
{
	initializeEmbedMethodsCommon<MExprLiteral>(embedName);
}
}; // namespace PatternMatcher