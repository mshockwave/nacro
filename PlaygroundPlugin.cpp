#include "clang/Basic/TokenKinds.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PPCallbacks.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include <memory>

using namespace clang;

struct ExamplePPCallbacks : public PPCallbacks {
  void MacroExpands(const Token& MacroNameToken,
                    const MacroDefinition& MD,
                    SourceRange Range,
                    const MacroArgs* Args) override {
    auto* MacroII = MacroNameToken.getIdentifierInfo();
    assert(MacroII);
    if(MacroII->isStr("hello")) {
      auto* MI = MD.getMacroInfo();
      auto SL = MI->getDefinitionLoc();
      Token Tok;
      Tok.startToken();
      Tok.setKind(tok::kw_long);
      Tok.setLocation(SL.getLocWithOffset(9));
      Tok.setLength(4);
      MI->AddTokenToBody(Tok);
      llvm::errs() << "onMacroExpands: "
                   << "Total of " << MI->getNumTokens() << " tokens\n";
    }
  }
};

class ExamplePragmaHandler : public PragmaHandler {
public:
  ExamplePragmaHandler() : PragmaHandler("example_pragma") { }
  void HandlePragma(Preprocessor &PP, PragmaIntroducer Introducer,
                    Token &PragmaTok) {
    llvm::SmallVector<Token, 16> Tokens;
    Token Tok;
    bool End = false;
    do {
      PP.Lex(Tok);
      Tokens.push_back(Tok);
      if(Tok.is(tok::identifier)) {
        auto* II = Tok.getIdentifierInfo();
        assert(II);
        if(II->getName() == "end")
          End = true;
      }
    } while(!End);

    llvm::errs() << "Tokens size = " << Tokens.size() << "\n";

    IdentifierInfo *RuleII = nullptr;
    // Find the first identifier, which is the rule name
    for(auto& TK : Tokens) {
      if(TK.is(tok::identifier)) {
        if(auto* II = TK.getIdentifierInfo()) {
          llvm::errs() << "Name: " << II->getName() << "\n";
          RuleII = II;
        }
        break;
      }
    }
    assert(RuleII);

    // Create a new macro
    auto SL = Introducer.Loc;
    auto* NewMI = PP.AllocateMacroInfo(SL);
    Tok.startToken();
    Tok.setKind(tok::kw_unsigned);
    Tok.setLocation(SL);
    Tok.setLength(8);
    NewMI->AddTokenToBody(Tok);

    PP.appendDefMacroDirective(RuleII, NewMI);

    // Add custom PPCallbacks
    auto PPC = std::make_unique<ExamplePPCallbacks>();
    PP.addPPCallbacks(std::move(PPC));
  }
};

static
PragmaHandlerRegistry::Add<ExamplePragmaHandler> Y("example_pragma",
                                                   "example pragma description");
