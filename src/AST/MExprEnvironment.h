#pragma once

#include "Expr.h"
#include "Embeddable.h" // for EmbedObject, etc.

#include "AST/MExpr.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace PatternMatcher
{
extern Expr MExprEnvironmentExpr();

 // Forward declaration
class MExpr;
class MExprSymbol;

class MExprEnvironment
{
private:
	std::unordered_map<std::string, std::weak_ptr<MExprSymbol>> symbol_cache;

	// Private constructor so no one can create it directly
	MExprEnvironment() = default;

public:
	// Delete copy/move semantics
	MExprEnvironment(const MExprEnvironment&) = delete;
	MExprEnvironment& operator=(const MExprEnvironment&) = delete;
	MExprEnvironment(MExprEnvironment&&) = delete;
	MExprEnvironment& operator=(MExprEnvironment&&) = delete;

	~MExprEnvironment() = default;

	static MExprEnvironment& instance()
	{
		static MExprEnvironment env; // Guaranteed to be created once (thread-safe since C++11)
		return env;
	}

	std::shared_ptr<MExprSymbol> getOrCreateSymbol(const Expr& e, const std::string& context,
												   const std::string& sourceName, bool isProtected);

	std::shared_ptr<MExpr> constructMExpr(Expr e);

	/// @brief Initialize the embedding in an expression.
	/// @param embedName The name to use for embedding.
	void initializeEmbedMethods(const char* embedName);
};

template <>
inline const char* EmbedName<MExprEnvironment>()
{
	return "PatternMatcherLibrary`AST`MExprEnvironment";
}

}; // namespace PatternMatcher