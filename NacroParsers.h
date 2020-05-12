#ifndef NACRO_NACROPARSERS_H
#define NACRO_NACROPARSERS_H
#include "clang/Basic/TokenKinds.h"
#include "clang/Lex/Preprocessor.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Optional.h"
#include "llvm/ADT/StringRef.h"
#include "NacroRule.h"
#include <memory>

namespace clang {
class NacroParser {
protected:
  /// #pragma nacro <category> [pragma params...]
  llvm::ArrayRef<Token> PragmaParams;

  Preprocessor& PP;

  bool HasEncounteredError;

  NacroParser(Preprocessor& PP, llvm::ArrayRef<Token> PragmaParams)
    : PragmaParams(PragmaParams),
      PP(PP),
      HasEncounteredError(false) {}

public:
  /// False if there is an error
  virtual operator bool() const {
    return !HasEncounteredError;
  }

  virtual bool Parse() = 0;
};

class NacroRuleParser : public NacroParser {
  // Owner of this instance
  std::unique_ptr<NacroRule> CurrentRule;

  /// All of the braces in a rule must match.
  /// This stack is used to reduce pair of braces.
  llvm::SmallVector<Token, 2> BraceStack;

public:
  NacroRuleParser(Preprocessor& PP, llvm::ArrayRef<Token> Params);

  NacroRule& getNacroRule() { return *CurrentRule; }
  std::unique_ptr<NacroRule> releaseNacroRule() {
    return std::move(CurrentRule);
  }

  bool ParseArgList();

  bool ParseStmts();

  llvm::Optional<NacroRule::Loop> ParseLoopHeader();
  bool ParseLoop(Token LoopTok);

  bool Parse() override;
};
} // end namespace clang
#endif
