#include "AST/MExprEnvironment.h"

#include "AST/MExpr.h"

#include "Expr.h"
#include "Logger.h"

#include <memory>
#include <optional>

namespace PatternMatcher
{

std::shared_ptr<MExpr> MExprEnvironment::constructMExpr(Expr e)
{
	return MExpr::construct(e);
}

namespace MethodInterface
{
	Expr constructMExpr(MExprEnvironment* env, Expr e)
	{
		return MExpr::toExpr(env->constructMExpr(e));
	}
}; // namespace MethodInterface

/// @brief Initialize the embedding in an expression.
/// @param embedName The name to use for embedding.
void MExprEnvironment::initializeEmbedMethods(const char* embedName)
{
	AddCompilerClassMethod_Export(
		embedName, "constructMExpr",
		reinterpret_cast<void*>(&embeddedObjectUnaryMethod<MExprEnvironment*, Expr, MethodInterface::constructMExpr>));
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