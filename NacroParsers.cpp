#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/DiagnosticSema.h"

#include "NacroParsers.h"

using namespace clang;

using llvm::StringRef;
using llvm::ArrayRef;
using llvm::Optional;

void NacroParser::CreateNewMacroDef(StringRef Name, ArrayRef<Token> Body) {
}

bool NacroRuleParser::ParseArgList() {
  Token Tok;
  Tok.startToken();

  do {
    PP.Lex(Tok);
    if(Tok.isNot(tok::identifier)) {
      PP.Diag(Tok, diag::err_expected) << "an identifier";
      return false;
    }
    auto* ArgII = Tok.getIdentifierInfo();
    assert(ArgII);

    PP.Lex(Tok);
    if(Tok.isNot(tok::colon)) {
      PP.Diag(Tok, diag::err_expected) << tok::colon;
      return false;
    }

    PP.Lex(Tok);
    if(Tok.isNot(tok::identifier)) {
      PP.Diag(Tok, diag::err_expected) << "argument type";
      return false;
    }
    auto* TII = Tok.getIdentifierInfo();
    assert(TII);
    auto RT = NacroRule::GetReplacementTy(TII->getName());
    if(RT == NacroRule::ReplacementTy::UNKNOWN) {
      PP.Diag(Tok, diag::err_unknown_typename) << TII->getName();
      return false;
    }

    bool isVarArgs = false;;
    PP.Lex(Tok);
    if(Tok.is(tok::star)) {
      // VARARGS
      isVarArgs = true;
      PP.Lex(Tok);
    }

    CurrentRule->AddReplacement(ArgII, RT, isVarArgs);

    if(Tok.is(tok::r_paren)) break;
    if(Tok.isNot(tok::comma)) {
      PP.Diag(Tok, diag::err_expected) << tok::comma;
      return false;
    }
  } while(true);

  return true;
}

bool NacroRuleParser::ParseStmts() {
  Token Tok;
  PP.Lex(Tok);
  if(Tok.isNot(tok::l_brace)) {
    PP.Diag(Tok, diag::err_expected) << tok::l_brace;
    return false;
  }
  CurrentRule->AddToken(Tok);
  BraceStack.push_back(Tok);

  while(!BraceStack.empty()) {
    Tok = PP.LookAhead(0);
    // Handle some special directives
    if(Tok.is(tok::identifier)) {
      auto* II = Tok.getIdentifierInfo();
      assert(II);
      if(II->isStr("$loop")) {
        if(!ParseLoop()) return false;
        continue;
      } else if(II->isStr("$str")) {
        // TODO
      }
    }

    PP.Lex(Tok);
    if(Tok.is(tok::eof)) {
      PP.Diag(Tok, diag::err_expected)
        << "'}'. Might be caused by mismatch braces";
      return false;
    }

    if(Tok.is(tok::r_brace)) {
      assert(!BraceStack.empty() &&
             "BraceStack is empty");
      auto TopTok = BraceStack.pop_back_val();
      if(TopTok.isNot(tok::l_brace)) {
        PP.Diag(TopTok, diag::err_expected)
          << "'{'. Might be caused by mismatch braces";
        return false;
      }
    }

    CurrentRule->AddToken(Tok);
  }

  return true;
}

Optional<NacroRule::Loop> NacroRuleParser::ParseLoopHeader() {
  using llvm::None;

  Token Tok;
  PP.Lex(Tok);
  if(Tok.isNot(tok::l_paren)) {
    PP.Diag(Tok, diag::err_expected) << tok::l_paren;
    return None;
  }

  // `$v in $range`
  PP.Lex(Tok);
  if(Tok.isNot(tok::identifier)) {
    PP.Diag(Tok, diag::err_expected)
      << "an identifier as the induction variable";
    return None;
  }
  auto* IV = Tok.getIdentifierInfo();
  assert(IV);

  PP.Lex(Tok);
  if(Tok.isNot(tok::identifier) ||
     !Tok.getIdentifierInfo() ||
     !Tok.getIdentifierInfo()->isStr("in")) {
    PP.Diag(Tok, diag::err_expected) << "keyword 'in'";
    return None;
  }

  // TODO: For now we only support single (varargs) variable as
  // the iteration range
  PP.Lex(Tok);
  if(Tok.isNot(tok::identifier)) {
    PP.Diag(Tok, diag::err_expected)
      << "an identifier as the iteration range";
    return None;
  }
  auto* IterRange = Tok.getIdentifierInfo();
  assert(IterRange);

  PP.Lex(Tok);
  if(Tok.isNot(tok::r_paren)) {
    PP.Diag(Tok, diag::err_expected) << tok::r_paren;
    return None;
  }

  return NacroRule::Loop{IV, IterRange};
}

bool NacroRuleParser::ParseLoop() {
  Token Tok;
  PP.Lex(Tok);
  assert(Tok.is(tok::identifier) && Tok.getIdentifierInfo());
  auto* II = Tok.getIdentifierInfo();
  assert(II->isStr("$loop"));

  auto LH = ParseLoopHeader();
  if(!LH) return false;

  // Use everything (especially the SrcLoc) from `$loop`
  // except the token kind
  Tok.setKind(tok::annot_pragma_loop_hint);
  CurrentRule->AddToken(Tok);
  auto LoopTokStartIdx = CurrentRule->token_size() - 1;

  // Parse the loop body
  if(!ParseStmts()) return false;

  Tok.startToken();
  Tok.setKind(tok::annot_pragma_loop_hint);
  CurrentRule->AddToken(Tok);
  auto LoopTokEndIdx = CurrentRule->token_size() - 1;

  CurrentRule->AddLoop(LoopTokStartIdx, LoopTokEndIdx, *LH);

  return true;
}

bool NacroRuleParser::Parse() {
  Token Tok;
  Tok.startToken();

  // '('
  PP.Lex(Tok);
  if(Tok.isNot(tok::l_paren)) {
    PP.Diag(Tok, diag::err_expected) << tok::l_paren;
    return false;
  }

  if(!ParseArgList()) return false;

  PP.Lex(Tok);
  if(Tok.isNot(tok::arrow)) {
    PP.Diag(Tok, diag::err_expected) << tok::arrow;
    return false;
  }

  return ParseStmts();
}
