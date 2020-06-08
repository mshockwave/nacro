#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/DiagnosticSema.h"

#include "NacroParsers.h"

using namespace clang;

using llvm::StringRef;
using llvm::ArrayRef;
using llvm::Optional;

NacroRuleParser::NacroRuleParser(Preprocessor& PP, ArrayRef<Token> Params)
  : NacroParser(PP, Params) {
  IdentifierInfo* NameII = nullptr;
  if(PragmaParams.size() > 0) {
    auto NameTok = PragmaParams[0];
    if(NameTok.isNot(tok::identifier)) {
      PP.Diag(NameTok, diag::err_expected) << "rule name identifier";
      HasEncounteredError = true;
    } else {
      NameII = NameTok.getIdentifierInfo();
      assert(NameII);
    }
  }
  CurrentRule = NacroRule::Create(NameII);
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
  if(CurTok.isNot(tok::l_brace)) {
    PP.Diag(CurTok, diag::err_expected) << tok::l_brace;
    return false;
  }
  CurrentRule->AddToken(CurTok);

  while(true) {
    Advance();

    // Handle some special directives
    if(CurTok.is(tok::identifier)) {
      auto* II = CurTok.getIdentifierInfo();
      assert(II);
      if(II->isStr("$loop")) {
        if(!ParseLoop()) return false;
        continue;
      } else if(II->isStr("$str")) {
        // TODO
      }
    }

    if(CurTok.is(tok::eof)) {
      PP.Diag(CurTok, diag::err_expected)
        << "'}'. Might be caused by mismatch braces";
      return false;
    }

    if(CurTok.is(tok::l_brace)) {
      if(!ParseStmts()) return false;
      continue;
    }

    CurrentRule->AddToken(CurTok);
    if(CurTok.is(tok::r_brace))
      return true;
  }
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
  assert(CurTok.is(tok::identifier));
  auto* II = CurTok.getIdentifierInfo();
  assert(II && II->isStr("$loop"));

  auto LH = ParseLoopHeader();
  if(!LH) return false;

  // Use everything (especially the SrcLoc) from `$loop`
  // except the token kind
  CurTok.setKind(tok::annot_pragma_loop_hint);
  CurrentRule->AddToken(CurTok);

  // Parse the loop body
  Advance();
  if(!ParseStmts()) return false;

  Token LoopEndTok;
  LoopEndTok.startToken();
  LoopEndTok.setKind(tok::annot_pragma_loop_hint);
  CurrentRule->AddToken(LoopEndTok);

  CurrentRule->AddLoop(*LH);

  return true;
}

bool NacroRuleParser::Parse() {
  if(HasEncounteredError) return false;

  Token Tok;
  Tok.startToken();

  // '('
  PP.Lex(Tok);
  if(Tok.isNot(tok::l_paren)) {
    PP.Diag(Tok, diag::err_expected) << tok::l_paren;
    return false;
  }

  auto BeginLoc = Tok.getLocation();

  if(!ParseArgList()) return false;

  PP.Lex(Tok);
  if(Tok.isNot(tok::arrow)) {
    PP.Diag(Tok, diag::err_expected) << tok::arrow;
    return false;
  }

  Advance();
  auto Res = ParseStmts();
  if(Res) {
    auto LastTok = CurrentRule->token_back();
    auto EndLoc = LastTok.getEndLoc();
    CurrentRule->setSourceRange(SourceRange(BeginLoc, EndLoc));
  }

  return Res;
}
