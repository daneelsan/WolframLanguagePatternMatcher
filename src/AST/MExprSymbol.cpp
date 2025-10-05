#include "AST/MExpr.h"

#include "Logger.h"

namespace PatternMatcher
{
void MExprSymbol::initializeEmbedMethods(const char* embedName)
{
	initializeEmbedMethodsCommon<MExprSymbol>(embedName);
}
}; // namespace PatternMatcher