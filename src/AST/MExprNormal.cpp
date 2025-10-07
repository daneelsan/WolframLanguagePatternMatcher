#include "AST/MExpr.h"

#include "Logger.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace PatternMatcher
{
std::shared_ptr<MExpr> MExprNormal::create(const Expr& e)
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

size_t MExprNormal::length() const
{
	return _children.size();
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
	// TODO: deep compare children
	return _expr.sameQ(o->_expr);
}

std::shared_ptr<MExpr> MExprNormal::part(mint i) const
{
	if (i < 0 || i > _children.size())
	{
		PM_ERROR("Index out of bounds in MExprNormal::part: ", i, " (length: ", _children.size(), ")");
		return nullptr;
	}
	if (i == 0)
	{
		return _head;
	}
	else
	{
		return _children[i - 1];
	}
}

namespace MethodInterface
{
	Expr part(MExprNormal* mexpr, mint i)
	{
		auto childMExpr = mexpr->part(i);
		return MExpr::toExpr(childMExpr);
	}
}; // namespace MethodInterface

void MExprNormal::initializeEmbedMethods(const char* embedName)
{
	initializeEmbedMethodsCommon<MExprNormal>(embedName);
	AddCompilerClassMethod_Export(
		embedName, "part",
		reinterpret_cast<void*>(&embeddedObjectUnaryMethod<std::shared_ptr<MExprNormal>, mint, MethodInterface::part>));
}
}; // namespace PatternMatcher