#include "llvm/ADT/StringSwitch.h"
#include "clang/Basic/IdentifierTable.h"

#include "NacroRule.h"

using namespace clang;

using llvm::StringRef;
using llvm::ArrayRef;

NacroRule::ReplacementTy NacroRule::GetReplacementTy(StringRef RawType) {
  return llvm::StringSwitch<ReplacementTy>(RawType)
          .Case("$expr", ReplacementTy::Expr)
          .Case("$ident", ReplacementTy::Ident)
          .Case("$stmt", ReplacementTy::Stmt)
          .Case("$block", ReplacementTy::Block)
          .Default(ReplacementTy::UNKNOWN);
}

void NacroRule::AddReplacement(IdentifierInfo* II, ReplacementTy Ty,
                               bool VarArgs) {
  Replacements.push_back({II, Ty, VarArgs});
}
