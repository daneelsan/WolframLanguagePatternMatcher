#include "AST/MExprPatternTools.h"
#include "AST/MExpr.h"

#include <memory>

namespace PatternMatcher
{
bool MExprIsBlank(std::shared_ptr<MExpr> patt)
{
	return patt->hasHead("Blank") and patt->length() <= 1;
}

bool MExprIsPattern(std::shared_ptr<MExpr> patt)
{
	if (patt->hasHead("Pattern") and patt->length() == 2)
	{
		auto pattNormal = std::static_pointer_cast<MExprNormal>(patt);
		return pattNormal->part(1)->getKind() == MExpr::Kind::Symbol;
	}
	return false;
}

bool MExprIsPatternTest(std::shared_ptr<MExpr> patt)
{
	return patt->hasHead("PatternTest") and patt->length() == 2;
}

bool MExprIsCondition(std::shared_ptr<MExpr> patt)
{
	return patt->hasHead("Condition") and patt->length() == 2;
}

bool MExprIsExcept(std::shared_ptr<MExpr> patt)
{
	mint len = patt->length();
	return patt->hasHead("Except") and (len == 1 or len == 2);
}

bool MExprIsAlternatives(std::shared_ptr<MExpr> patt)
{
	return patt->hasHead("Alternatives") and patt->length() >= 1;
}

bool MExprIsRepeated(std::shared_ptr<MExpr> patt)
{
	mint len = patt->length();
	return patt->hasHead("Repeated") and (len == 1 or len == 2);
}

bool MExprIsRepeatedNull(std::shared_ptr<MExpr> patt)
{
	mint len = patt->length();
	return patt->hasHead("RepeatedNull") and (len == 1 or len == 2);
}

bool MExprIsBlankSequence(std::shared_ptr<MExpr> patt)
{
	return patt->hasHead("BlankSequence") and patt->length() <= 1;
}

bool MExprIsBlankNullSequence(std::shared_ptr<MExpr> patt)
{
	return patt->hasHead("BlankNullSequence") and patt->length() <= 1;
}

bool MExprIsPatternSequence(std::shared_ptr<MExpr> patt)
{
	return patt->hasHead("PatternSequence");
}
}; // namespace PatternMatcher