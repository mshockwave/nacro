#include "llvm/ADT/StringSwitch.h"
#include "clang/Basic/IdentifierTable.h"
#include "NacroRule.h"
#include <memory>
#include <vector>

using namespace clang;

using llvm::StringRef;
using llvm::ArrayRef;

static std::vector<std::unique_ptr<NacroRule>> NacroRulesOwner;

NacroRule* NacroRule::Create(IdentifierInfo* NameII) {
  NacroRulesOwner.emplace_back(new NacroRule(NameII));
  return NacroRulesOwner.back().get();
}

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

bool NacroRule::needsPPHooks() const {
  // For now only loops need PPCallbacks
  return !loop_empty();
}
