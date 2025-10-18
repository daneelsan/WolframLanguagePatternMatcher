#include "AST/MExpr.h"
#include "AST/MExprEnvironment.h"

#include "Logger.h"

#include <memory>
#include <string>

namespace PatternMatcher
{
// Get the symbol context, e.g. "System`" or "Global`"
// Returns empty string on failure (caller should handle)
static std::string getSymbolContext(const Expr& e)
{
	auto contextExpr = Expr::construct("Context", e).eval();
	auto contextOpt = contextExpr.as<std::string>();
	if (!contextOpt)
	{
		PM_ERROR("getSymbolContext failed for: ", e);
		return "";
	}
	return *contextOpt;
}

// Return whether the symbol has the Protected attribute.
// If something goes wrong, default to false.
static bool isProtectedSymbol(const Expr& e)
{
	auto attrExpr = Expr::construct("Attributes", e).eval();
	auto res = Expr::construct("MemberQ", attrExpr, Expr::ToExpression("Protected")).eval();
	return res.as<bool>().value_or(false);
}

std::shared_ptr<MExpr> MExprSymbol::create(const Expr& e)
{
	std::string context = e.context().value();
	std::string sourceName = e.symbolName().value();
	bool isSystem = context == "System`";
	bool isProtected = e.protectedQ().value();
	MExprEnvironment* env = &MExprEnvironment::instance();
	return env->getOrCreateSymbol(e, context, sourceName, isSystem && isProtected);
}

std::shared_ptr<MExpr> MExprSymbol::getHead() const
{
	return MExprSymbol::create(Expr::ToExpression("Symbol"));
}

bool MExprSymbol::sameQ(std::shared_ptr<MExpr> other) const
{
	if (other->getKind() != Kind::Symbol)
		return false;
	auto o = std::static_pointer_cast<MExprSymbol>(other);
	return _expr.sameQ(o->_expr) && name == o->name && context == o->context;
}

namespace MExprSymbolInterface
{
	Expr getLexicalName(std::shared_ptr<MExprSymbol> obj)
	{
		return Expr(obj->getLexicalName());
	}
	Expr toBoxes(Expr objExpr, Expr fmt)
	{
		return Expr::construct("DanielS`PatternMatcher`AST`Private`toMExprSymbolBoxes", objExpr, fmt).eval();
	}
}; // namespace MExprSymbolInterface

void MExprSymbol::initializeEmbedMethods(const char* embedName)
{
	initializeEmbedMethodsCommon<MExprSymbol>(embedName);
	RegisterMethod<std::shared_ptr<MExprSymbol>, MExprSymbolInterface::getLexicalName>(embedName, "getLexicalName");
	RegisterMethod<std::shared_ptr<MExprSymbol>, MExprSymbolInterface::toBoxes>(embedName, "toBoxes");
}
}; // namespace PatternMatcher