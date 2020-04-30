#ifndef NACRO_NACROPARSERS_H
#define NACRO_NACROPARSERS_H
#include "clang/Basic/TokenKinds.h"
#include "clang/Lex/Preprocessor.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "NacroRule.h"
#include <memory>

namespace clang {
class NacroParser {
protected:
  llvm::ArrayRef<Token> PragmaParams;

  Preprocessor& PP;

  NacroParser(Preprocessor& PP, llvm::ArrayRef<Token> PragmaParams)
    : PragmaParams(PragmaParams),
      PP(PP) {}

  void CreateNewMacroDef(llvm::StringRef Name, llvm::ArrayRef<Token> Body);

public:
  virtual void Parse() = 0;
};

class NacroRuleParser : public NacroParser {
  // Owner of this instance
  std::unique_ptr<NacroRule> CurrentRule;

public:
  NacroRuleParser(Preprocessor& PP, llvm::ArrayRef<Token> Params)
    : NacroParser(PP, Params),
      CurrentRule(new NacroRule()) {}

  NacroRule& getNacroRule() { return *CurrentRule; }

  bool ParseArgList();

  void Parse() override;
};
} // end namespace clang
#endif
