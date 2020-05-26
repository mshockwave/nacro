#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/MacroArgs.h"
#include "clang/Lex/PPCallbacks.h"
#include "NacroExpanders.h"
#include <iterator>
#include <vector>

using namespace clang;
using llvm::ArrayRef;
using llvm::Error;

void NacroRuleExpander::CreateMacroDirective(IdentifierInfo* Name,
                                             SourceLocation BeginLoc,
                                             ArrayRef<IdentifierInfo*> Args,
                                             ArrayRef<Token> Body) {
  auto* MI = PP.AllocateMacroInfo(BeginLoc);
  MI->setIsFunctionLike();

  for(auto Tok : Body) {
    MI->AddTokenToBody(Tok);
  }

  MI->setParameterList(Args, PP.getPreprocessorAllocator());
  PP.appendDefMacroDirective(Name, MI);
}

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

namespace {
struct LoopExpandingPPCallbacks : public PPCallbacks {
  LoopExpandingPPCallbacks(NacroRule& Rule, Preprocessor& PP)
    : Rule(Rule), PP(PP) {
    assert(Rule.hasVAArgs() && "No loops to expand from arguments");
  }

  void ExpandsLoop(const NacroRule::Loop& LoopInfo,
                   ArrayRef<Token> LoopBody,
                   ArrayRef<std::vector<Token>> FormalArgs,
                   SmallVectorImpl<Token>& OutputBuffer) {
  }

  void MacroExpands(const Token& MacroNameToken,
                    const MacroDefinition& MD,
                    SourceRange Range,
                    const MacroArgs* ConstArgs) override {
    // FIXME: Is this safe?
    auto* Args = const_cast<MacroArgs*>(ConstArgs);
    auto* MacroII = MacroNameToken.getIdentifierInfo();
    assert(MacroII);
    if(MacroII != Rule.getName()) return;
    assert(Args->getNumMacroArguments() == Rule.replacements_size());

    auto* MI = MD.getMacroInfo();
    // If there is a VAArgs, it must be the last (formal) argument
    auto VAArgsIdx = Rule.replacements_size() - 1;
    auto& VAReplacement = Rule.getReplacement(VAArgsIdx);
    const auto& RawExpVAArgs
      = Args->getPreExpArgument(VAArgsIdx, PP);
    SmallVector<std::vector<Token>, 4> ExpVAArgs;
    for(int Start = 0, i = 0; i < RawExpVAArgs.size(); ++i) {
      auto Tok = RawExpVAArgs[i];
      if(Tok.isOneOf(tok::eof, tok::comma)) {
        std::vector<Token> Buffer;
        for(; Start < i; ++Start) {
          Buffer.push_back(RawExpVAArgs[Start]);
        }
        ExpVAArgs.push_back(std::move(Buffer));
        ++Start;
      }
    }

    SmallVector<Token, 16> ExpTokens;
    for(auto TokIdx = 0; TokIdx < Rule.token_size(); ++TokIdx) {
      auto Tok = Rule.getToken(TokIdx);
      if(Tok.is(tok::annot_pragma_loop_hint)) {
        auto LPI = Rule.FindLoop(TokIdx);
        assert(LPI != Rule.loop_end() &&
               "Not in the loop map?");
        assert(LPI.start() == TokIdx &&
               "Not pointing to the first loop token?");
        const auto& LP = LPI.value();
        assert(VAReplacement.Identifier == LP.IterRange &&
               "Iterating on non VAArgs variable");

        // Extract loop body
        SmallVector<Token, 8> LoopBody;
        for(auto E = LPI.stop(); TokIdx <= E; ++TokIdx) {
          Tok = Rule.getToken(TokIdx);
          if(Tok.isNot(tok::annot_pragma_loop_hint)) {
            LoopBody.push_back(Tok);
          }
        }
        ExpandsLoop(LP, LoopBody, ExpVAArgs, ExpTokens);
      } else {
        ExpTokens.push_back(Tok);
      }
    }
  }

private:
  NacroRule& Rule;
  Preprocessor& PP;
};
} // end anonymous namespace

Error NacroRuleExpander::LoopExpanding() {
  return Error::success();
}

Error NacroRuleExpander::Expand() {
  if(auto E = ReplacementProtecting())
    return E;

  if(!Rule.needsPPHooks()) {
    // export as a normal macro function
    llvm::SmallVector<IdentifierInfo*, 2> ReplacementsII;
    llvm::transform(Rule.replacements(), std::back_inserter(ReplacementsII),
                    [](NacroRule::Replacement& R) { return R.Identifier; });
    CreateMacroDirective(Rule.getName(), Rule.getBeginLoc(),
                         ReplacementsII,
                         ArrayRef<Token>(Rule.token_begin(), Rule.token_end()));
  } else if(!Rule.loop_empty()) {
    if(auto E = LoopExpanding())
      return E;
  }
  return Error::success();
}
