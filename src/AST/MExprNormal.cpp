#include "AST/MExpr.h"

#include "Logger.h"

namespace PatternMatcher
{
std::shared_ptr<MExpr> MExprNormal::create(Expr e)
{
	// Normal expressions: head + children
    mint len = e.length();
	Expr headExpr = e.head();
	auto headMExpr = MExpr::construct(headExpr);
	std::vector<std::shared_ptr<MExpr>> children;
	children.reserve(len);
	for (mint i = 1; i <= len; i += 1)
	{
		Expr child = e.part(i);
		children.push_back(MExpr::construct(child));
	}
	return std::make_shared<MExprNormal>(e, headMExpr, std::move(children));
}

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