#include "AST/MExprEnvironment.h"

#include "AST/MExpr.h"

#include "Expr.h"
#include "Logger.h"

#include <memory>
#include <optional>
#include <string>

namespace PatternMatcher
{
std::shared_ptr<MExprSymbol> MExprEnvironment::getOrCreateSymbol(const Expr& e, const std::string& context,
																 const std::string& sourceName, bool isSystemProtected)
{
	if (isSystemProtected)
	{
		auto it = symbol_cache.find(sourceName);
		if (it != symbol_cache.end())
		{
			if (auto cached = it->second.lock())
			{
				return cached;
			}
		}

		auto mexpr = std::make_shared<MExprSymbol>(e, context, sourceName, true);
		symbol_cache[sourceName] = mexpr;
		return mexpr;
	}

	// Non-protected symbols are not cached
	return std::make_shared<MExprSymbol>(e, context, sourceName, false);
}

std::shared_ptr<MExpr> MExprEnvironment::constructMExpr(const Expr& e)
{
	return MExpr::construct(e);
}

namespace MethodInterface
{
	// This method assumes the passed expression is HoldComplete[expr].
	Expr constructMExpr(MExprEnvironment* env, Expr heldExpr)
	{
		// Get the unheld expression
		auto expr = heldExpr.part(1);
		return MExpr::toExpr(env->constructMExpr(expr));
	}
}; // namespace MethodInterface

/// @brief Initialize the embedding in an expression.
/// @param embedName The name to use for embedding.
void MExprEnvironment::initializeEmbedMethods(const char* embedName)
{
	RegisterMethod<MExprEnvironment*, MethodInterface::constructMExpr>(embedName, "constructMExpr");
};

/// @brief Get the Expr representing the singleton MExprEnvironment instance.
/// @return An Expr embedding the singleton MExprEnvironment instance.
Expr MExprEnvironmentExpr()
{
	// Use the singleton instance
	MExprEnvironment* env = &MExprEnvironment::instance();
	return EmbedObject(env);
}

}; // namespace PatternMatcher