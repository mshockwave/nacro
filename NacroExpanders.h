#ifndef NACRO_NACRO_EXPANDERS_H
#define NACRO_NACRO_EXPANDERS_H
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/Error.h"
#include "clang/Lex/Preprocessor.h"
#include "NacroRule.h"

namespace clang {
/// Inject nacro definition into token stream.
/// Currently it will do some simple protections, adding
/// paran for expressions for example. As well as instantiating
/// loops.
class NacroRuleExpander {
  NacroRule* Rule;

  Preprocessor& PP;

public:
  NacroRuleExpander(NacroRule* Rule, Preprocessor& PP)
    : Rule(Rule),
      PP(PP) {}

  llvm::Error ReplacementProtecting();

  llvm::Error Expand();

  inline NacroRule* getNacroRule() {
    return Rule;
  }
};
} // end namespace clang
#endif
