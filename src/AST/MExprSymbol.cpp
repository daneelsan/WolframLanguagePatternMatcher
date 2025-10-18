#include "AST/MExpr.h"
#include "AST/MExprEnvironment.h"

#include "Logger.h"

#include <memory>
#include <string>

namespace PatternMatcher
{
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
	Expr getContext(std::shared_ptr<MExprSymbol> obj) {
		return Expr(obj->getContext());
	}
	Expr getLexicalName(std::shared_ptr<MExprSymbol> obj)
	{
		return Expr(obj->getLexicalName());
	}
	Expr getName(std::shared_ptr<MExprSymbol> obj) {
		return Expr(obj->getName());
	}
	Expr getSourceName(std::shared_ptr<MExprSymbol> obj) {
		return Expr(obj->getSourceName());
	}
	Expr isSystemProtected(std::shared_ptr<MExprSymbol> obj) {
		return toExpr(obj->isSystemProtected());
	}
	Expr toBoxes(Expr objExpr, Expr fmt)
	{
		return Expr::construct("DanielS`PatternMatcher`AST`Private`toMExprSymbolBoxes", objExpr, fmt).eval();
	}
}; // namespace MExprSymbolInterface

void MExprSymbol::initializeEmbedMethods(const char* embedName)
{
	initializeEmbedMethodsCommon<MExprSymbol>(embedName);
	RegisterMethod<std::shared_ptr<MExprSymbol>, MExprSymbolInterface::getContext>(embedName, "getContext");
	RegisterMethod<std::shared_ptr<MExprSymbol>, MExprSymbolInterface::getLexicalName>(embedName, "getLexicalName");
	RegisterMethod<std::shared_ptr<MExprSymbol>, MExprSymbolInterface::getName>(embedName, "getName");
	RegisterMethod<std::shared_ptr<MExprSymbol>, MExprSymbolInterface::getSourceName>(embedName, "getSourceName");
	RegisterMethod<std::shared_ptr<MExprSymbol>, MExprSymbolInterface::isSystemProtected>(embedName, "isSystemProtected");
	RegisterMethod<std::shared_ptr<MExprSymbol>, MExprSymbolInterface::toBoxes>(embedName, "toBoxes");
}
}; // namespace PatternMatcher