#include "AST/MExpr.h"

#include "Logger.h"

namespace PatternMatcher
{
std::shared_ptr<MExpr> MExprLiteral::create(const Expr& e)
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
	{
		return false;
	}
	auto o = std::static_pointer_cast<MExprLiteral>(other);
	if (getID() == o->getID())
	{
		return true;
	}
	return _expr.sameQ(o->_expr);
}

namespace MExprLiteralInterface
{
	Expr toBoxes(Expr objExpr, Expr fmt)
	{
		return Expr::construct("DanielS`PatternMatcher`AST`Private`toMExprLiteralBoxes", objExpr, fmt).eval();
	}
}; // namespace MExprLiteralInterface

void MExprLiteral::initializeEmbedMethods(const char* embedName)
{
	initializeEmbedMethodsCommon<MExprLiteral>(embedName);
	RegisterMethod<std::shared_ptr<MExprLiteral>, MExprLiteralInterface::toBoxes>(embedName, "toBoxes");
}
}; // namespace PatternMatcher