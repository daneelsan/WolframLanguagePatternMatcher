#pragma once

#include "AST/MExpr.h"

#include <memory>

// Pattern matching utilities for MExpr patterns

namespace PatternMatcher
{
bool MExprIsBlank(std::shared_ptr<MExpr> patt);

bool MExprIsPattern(std::shared_ptr<MExpr> patt);

bool MExprIsPatternTest(std::shared_ptr<MExpr> patt);

bool MExprIsCondition(std::shared_ptr<MExpr> patt);

bool MExprIsExcept(std::shared_ptr<MExpr> patt);

bool MExprIsAlternatives(std::shared_ptr<MExpr> patt);

bool MExprIsRepeated(std::shared_ptr<MExpr> patt);

bool MExprIsRepeatedNull(std::shared_ptr<MExpr> patt);

bool MExprIsBlankSequence(std::shared_ptr<MExpr> patt);

bool MExprIsBlankNullSequence(std::shared_ptr<MExpr> patt);

bool MExprIsPatternSequence(std::shared_ptr<MExpr> patt);
}; // namespace PatternMatcher