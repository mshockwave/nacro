#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "clang/Lex/MacroInfo.h"
#include "NacroExpander.h"
#include <iterator>

using namespace clang;
using llvm::Error;

Error NacroRuleExpander::ReplacementProtecting() {
  using namespace llvm;
  DenseMap<IdentifierInfo*, typename NacroRule::Replacement> IdentMap;
  for(auto& R : Rule.replacements()) {
    if(R.Identifier && !R.VarArgs) {
      IdentMap.insert({R.Identifier, R});
    }
  }
  if(IdentMap.empty()) return Error::success();

  for(auto TI = Rule.token_begin(); TI != Rule.token_end();) {
    auto Tok = *TI;
    auto TokLoc = Tok.getLocation();
    if(Tok.is(tok::identifier)) {
      auto* II = Tok.getIdentifierInfo();
      assert(II);
      if(IdentMap.count(II)) {
        auto& R = IdentMap[II];
        using RTy = typename NacroRule::ReplacementTy;
        switch(R.Type) {
        case RTy::Expr: {
          ++TI;
          Token LParen, RParen;
          RParen.startToken();
          RParen.setKind(tok::r_paren);
          RParen.setLength(1);
          RParen.setLocation(TokLoc.getLocWithOffset(1));
          TI = Rule.insert_token(TI, RParen);
          --TI;
          LParen.startToken();
          LParen.setKind(tok::l_paren);
          LParen.setLength(1);
          LParen.setLocation(TokLoc.getLocWithOffset(-1));
          TI = Rule.insert_token(TI, LParen);
          // skip expr and r_paren
          for(int i = 0; i < 3; ++i) ++TI;
          break;
        }
        case RTy::Stmt: {
          ++TI;
          Token Semi;
          Semi.startToken();
          Semi.setKind(tok::semi);
          Semi.setLength(1);
          Semi.setLocation(TokLoc.getLocWithOffset(1));
          TI = Rule.insert_token(TI, Semi);
          ++TI;
          break;
        }
        default: ++TI;
        }
        continue;
      }
    }
    ++TI;
  }
  return Error::success();
}

Error NacroRuleExpander::LoopExpanding() {
  // TODO
  return Error::success();
}

Error NacroRuleExpander::Expand() {
  if(auto E = ReplacementProtecting())
    return E;

  if(!Rule.needsPPHooks()) {
    // export as a normal macro function
    auto* MI = PP.AllocateMacroInfo(Rule.getBeginLoc());
    MI->setIsFunctionLike();

    for(auto Tok : Rule.tokens()) {
      MI->AddTokenToBody(Tok);
    }

    llvm::SmallVector<IdentifierInfo*, 2> ReplacementsII;
    llvm::transform(Rule.replacements(), std::back_inserter(ReplacementsII),
                    [](NacroRule::Replacement& R) { return R.Identifier; });
    MI->setParameterList(ReplacementsII, PP.getPreprocessorAllocator());

    PP.appendDefMacroDirective(Rule.getName(), MI);
  } else if(!Rule.loop_empty()) {
    if(auto E = LoopExpanding())
      return E;
  }
  return Error::success();
}
