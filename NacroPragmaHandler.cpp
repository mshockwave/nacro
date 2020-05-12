#include "clang/Basic/TokenKinds.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PPCallbacks.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"

#include "NacroParsers.h"
#include "NacroExpander.h"
#include <memory>

using namespace clang;

namespace {
struct NacroPragmaHandler : public PragmaHandler {
  NacroPragmaHandler() : PragmaHandler("nacro") {}

  void HandlePragma(Preprocessor &PP, PragmaIntroducer Introducer,
                    Token &PragmaTok) override;
};

void NacroPragmaHandler::HandlePragma(Preprocessor &PP,
                                      PragmaIntroducer Introducer,
                                      Token &PragmaTok) {
  Token Tok;
  Tok.startToken();

  // Decide the category
  // (Currently we have only one category: rule)
  StringRef Category;
  SmallVector<Token, 1> PragmaArgs;
  PP.Lex(Tok);
  if(Tok.isNot(tok::identifier)) {
    llvm::errs() << "Expecting nacro category\n";
    return;
  }
  auto* II = Tok.getIdentifierInfo();
  assert(II);
  Category = II->getName();
  do {
    PP.Lex(Tok);
    PragmaArgs.push_back(Tok);
  } while(Tok.isNot(tok::eod));

  if(Category == "rule") {
    NacroRuleParser Parser(PP, PragmaArgs);
    if(!Parser.Parse()) return;
    auto Rule = Parser.releaseNacroRule();

    NacroRuleExpander Expander(*Rule, PP);
    if(Expander.Expand()) return;
  } else {
    llvm::errs() << "Unrecognized category: "
                 << Category << "\n";
  }
}

PragmaHandlerRegistry::Add<NacroPragmaHandler> X("nacro", "New Macro");
} // end anonymous namespace
