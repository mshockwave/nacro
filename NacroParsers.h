#ifndef NACRO_NACRO_PARSERS_H
#define NACRO_NACRO_PARSERS_H
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
  NacroRule* CurrentRule;

  Token CurTok;

public:
  NacroRuleParser(Preprocessor& PP, llvm::ArrayRef<Token> Params);

  inline
  NacroRule* getNacroRule() { return CurrentRule; }

  inline void Advance() {
    PP.Lex(CurTok);
  }

  bool ParseArgList();

  bool ParseStmts();

  llvm::Optional<NacroRule::Loop> ParseLoopHeader();
  bool ParseLoop();

  bool Parse() override;
};
} // end namespace clang
#endif
