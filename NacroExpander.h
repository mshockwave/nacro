#include "llvm/Support/Error.h"
#include "clang/Lex/Preprocessor.h"
#include "NacroRule.h"

namespace clang {
/// Inject nacro definition into token stream.
/// Currently it will do some simple protections, adding
/// paran for expressions for example. As well as instantiating
/// loops.
class NacroRuleExpander {
  NacroRule& Rule;

  Preprocessor& PP;

public:
  NacroRuleExpander(NacroRule& Rule, Preprocessor& PP)
    : Rule(Rule),
      PP(PP) {}

  llvm::Error ReplacementProtecting();

  llvm::Error LoopExpanding();

  llvm::Error Expand();
};
} // end namespace clang
