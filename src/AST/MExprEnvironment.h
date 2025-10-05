#pragma once

#include "Expr.h"
#include "Embeddable.h" // for EmbedObject, etc.

#include "AST/MExpr.h"

namespace PatternMatcher
{
extern Expr MExprEnvironmentExpr();

class MExprEnvironment
{
private:
	size_t id;

public:
	MExprEnvironment() { id = 0; }
	~MExprEnvironment() = default;

	/// @brief Initialize the embedding in an expression.
	/// @param embedName The name to use for embedding.
	void initializeEmbedMethods(const char* embedName);

	std::shared_ptr<MExpr> createMExpr(Expr e);
};

template <>
inline const char* EmbedName<MExprEnvironment>()
{
	return "PatternMatcherLibrary`AST`MExprEnvironment";
}

}; // namespace PatternMatcher