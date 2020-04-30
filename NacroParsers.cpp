#include "clang/Basic/DiagnosticIDs.h"
#include "clang/Basic/DiagnosticSema.h"

#include "NacroParsers.h"

using namespace clang;

using llvm::StringRef;
using llvm::ArrayRef;

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

void NacroRuleParser::Parse() {
  Token Tok;
  Tok.startToken();

  // '('
  PP.Lex(Tok);
  if(Tok.isNot(tok::l_paren)) {
    PP.Diag(Tok, diag::err_expected) << tok::l_paren;
    return;
  }
  if(!ParseArgList()) return;

  PP.Lex(Tok);
  if(Tok.isNot(tok::arrow)) {
    PP.Diag(Tok, diag::err_expected) << tok::arrow;
    return;
  }
}
