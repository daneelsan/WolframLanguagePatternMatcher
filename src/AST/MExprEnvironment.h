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

	/// @brief Get the singleton instance of MExprEnvironment.
	/// @details This method returns a reference to the singleton instance of MExprEnvironment.
	/// It is thread-safe and guarantees that only one instance is created.
	/// @return A reference to the singleton MExprEnvironment instance.
	static MExprEnvironment& instance()
	{
		static MExprEnvironment env; // Guaranteed to be created once (thread-safe since C++11)
		return env;
	}

	/// @brief Get or create a symbol MExpr.
	/// @param e The Expr representing the symbol.
	/// @param context The context of the symbol.
	/// @param sourceName The name of the symbol as it appears on the source.
	/// @param isSystemProtected Whether the symbol is a protected System` symbol.
	/// @return A shared_ptr to the symbol MExpr.
	std::shared_ptr<MExprSymbol> getOrCreateSymbol(const Expr& e, const std::string& context,
												   const std::string& sourceName, bool isSystemProtected);

	/// @brief Construct a new MExpr from an Expr.
	/// @param e The Expr to convert.
	/// @return A shared_ptr to the new MExpr.
	std::shared_ptr<MExpr> constructMExpr(const Expr& e);

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