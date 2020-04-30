#ifndef NACRO_NACRORULE_H
#define NACRO_NACRORULE_H
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"

namespace clang {
// Forward Declarations
class IdentifierInfo;

struct NacroRule {
  enum class ReplacementTy {
    UNKNOWN,
    Expr,
    Ident,
    Stmt,
    Block
  };
  static ReplacementTy GetReplacementTy(llvm::StringRef RawType);

  struct Replacement {
    IdentifierInfo* Identifier;
    ReplacementTy Type;
    bool VarArgs;
  };

  void AddReplacement(IdentifierInfo* II, ReplacementTy Ty,
                      bool VarArgs = false);

private:
  /// a.k.a Macro rule arguments
  llvm::SmallVector<Replacement, 2> Replacements;

public:
  using repl_iterator
    = typename decltype(Replacements)::iterator;

  repl_iterator replacement_begin() {
    return Replacements.begin();
  }

  repl_iterator replacement_end() {
    return Replacements.end();
  }

  size_t replacements_size() const {
    return Replacements.size();
  }
};
} // end namespace clang
#endif
