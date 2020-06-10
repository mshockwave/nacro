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
        if(!ParseStr()) return false;
        continue;
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

bool NacroRuleParser::ParseStr() {
  assert(CurTok.is(tok::identifier));
  assert(CurTok.getIdentifierInfo() &&
         CurTok.getIdentifierInfo()->isStr("$str"));

  PP.Lex(CurTok);
  if(CurTok.isNot(tok::l_paren)) {
    PP.Diag(CurTok, diag::err_expected) << tok::l_paren;
    return false;
  }
  // We need their Location info
  Token LParenTok = CurTok;

  PP.Lex(CurTok);
  Token StrTok = CurTok;

  PP.Lex(CurTok);
  if(CurTok.isNot(tok::r_paren)) {
    PP.Diag(CurTok, diag::err_expected) << tok::r_paren;
    return false;
  }

  LParenTok.setKind(tok::hash);
  CurrentRule->AddToken(LParenTok);
  CurrentRule->AddToken(StrTok);

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

/// Wrap body with correct enclosment symbols
/// based on generated type
void NacroRuleParser::WrapNacroBody() {
  switch(CurrentRule->getGeneratedType()) {
  case NacroRule::ReplacementTy::Expr: {
    // replace with parens
    Token FirstTok = CurrentRule->getToken(0),
          LastTok = CurrentRule->token_back();
    FirstTok.setKind(tok::l_paren);
    LastTok.setKind(tok::r_paren);

    auto Head = CurrentRule->erase_token(CurrentRule->token_begin());
    CurrentRule->insert_token(Head, FirstTok);
    auto Tail = CurrentRule->insert_token(CurrentRule->token_end(),
                                          LastTok);
    CurrentRule->erase_token(--Tail);
    break;
  }
  case NacroRule::ReplacementTy::Stmt: {
    // remove any enclosement and add semi-colon at the end
    // if there hasn't any
    CurrentRule->erase_token(CurrentRule->token_begin());

    auto LastTok = CurrentRule->token_back();
    LastTok.setKind(tok::semi);
    auto Tail = CurrentRule->insert_token(CurrentRule->token_end(),
                                          LastTok);
    Tail = CurrentRule->erase_token(--Tail);
    if((--Tail)->is(tok::semi)) {
      // has duplicate, remove the last one
      CurrentRule->erase_token(++Tail);
    }
    break;
  }
  case NacroRule::ReplacementTy::Block:
    // block will remain the same
    return;
  }
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

  // first token of the entire nacro, including argument list
  auto FirstTok = Tok;

  if(!ParseArgList()) return false;

  // '->'
  PP.Lex(Tok);
  if(Tok.isNot(tok::arrow)) {
    PP.Diag(Tok, diag::err_expected) << tok::arrow;
    return false;
  }

  // generated type
  Advance();
  if(CurTok.is(tok::identifier)) {
    // Explicitily specified the generated type
    auto* GII = CurTok.getIdentifierInfo();
    assert(GII);
    auto GT = NacroRule::GetReplacementTy(GII->getName());
    if(GT == NacroRule::ReplacementTy::UNKNOWN) {
      PP.Diag(Tok, diag::err_unknown_typename) << GII->getName();
      return false;
    }
    CurrentRule->setGeneratedType(GT);
    Advance();
  }

  auto Res = ParseStmts();
  if(Res) {
    WrapNacroBody();

    auto LastTok = CurrentRule->token_back();
    CurrentRule->setSourceRange(SourceRange(FirstTok.getLocation(),
                                            LastTok.getEndLoc()));
  }

  return Res;
}
