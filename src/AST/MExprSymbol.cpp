#include "AST/MExpr.h"
#include "AST/MExprEnvironment.h"

#include "Logger.h"

#include <memory>
#include <string>

namespace PatternMatcher
{
// TODO: Consider if the Expr should be passed by reference instead
// This should avoid unnecessary refcounting.
static std::string getSymbolContext(Expr e)
{
	auto contextExpr = Expr::construct("Context", e).eval();
	auto contextOpt = contextExpr.as<std::string>();
	// TODO: assert here
	return *contextOpt;
}

static bool isProtectedSymbol(Expr e)
{
	auto attrExpr = Expr::construct("Attributes", e).eval();
	auto res = Expr::construct("MemberQ", attrExpr, Expr::inertExpression("Protected")).eval();
	return res.as<bool>().value_or(false);
}

std::shared_ptr<MExpr> MExprSymbol::create(Expr e)
{
	std::string context = getSymbolContext(e);
	std::string sourceName = e.toString();
	bool isSystem = context == "System`";
	bool isProtected = isProtectedSymbol(e);
	MExprEnvironment* env = &MExprEnvironment::instance();
	return env->getOrCreateSymbol(e, context, sourceName, isSystem && isProtected);
}

std::shared_ptr<MExpr> MExprSymbol::getHead() const
{
	return MExpr::construct(Expr::inertExpression("Symbol"));
}

bool MExprSymbol::sameQ(std::shared_ptr<MExpr> other) const
{
	if (other->getKind() != Kind::Symbol)
		return false;
	auto o = std::static_pointer_cast<MExprSymbol>(other);
	return _expr.sameQ(o->_expr) && name == o->name && context == o->context;
}

namespace MethodInterface
{

}; // namespace MethodInterface

void MExprSymbol::initializeEmbedMethods(const char* embedName)
{
	initializeEmbedMethodsCommon<MExprSymbol>(embedName);
}
}; // namespace PatternMatcher