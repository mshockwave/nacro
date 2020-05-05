#ifndef NACRO_NACRORULE_H
#define NACRO_NACRORULE_H
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/IntervalMap.h"
#include "clang/Lex/Preprocessor.h"

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

  void AddToken(const Token& Tok) {
    Tokens.push_back(Tok);
  }

  struct Loop {
    IdentifierInfo* InductionVar;
    IdentifierInfo* IterRange;

    inline
    bool operator==(const Loop& RHS) const {
      return IterRange == RHS.IterRange &&
             InductionVar == RHS.InductionVar;
    }
    inline
    bool operator!=(const Loop& RHS) const {
      return !(*this == RHS);
    }
  };

private:
  /// a.k.a Macro rule arguments
  llvm::SmallVector<Replacement, 2> Replacements;

  /// Note that a loop region is surrounded by a pair of
  /// tok::annot_pragma_loop_hint
  llvm::SmallVector<Token, 16> Tokens;

  using LoopIntervalsTy = llvm::IntervalMap<size_t, Loop>;
  // FIXME: Do we need to hoist allocator to the upper level?
  // i.e. shared a single allocator among rules
  LoopIntervalsTy::Allocator LoopIntervalAlloc;
  LoopIntervalsTy Loops;

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

  using token_iterator =
    typename decltype(Tokens)::const_iterator;

  token_iterator token_begin() const {
    return Tokens.begin();
  }

  token_iterator token_end() const {
    return Tokens.end();
  }

  size_t token_size() const { return Tokens.size(); }

  NacroRule()
    : LoopIntervalAlloc(),
      Loops(LoopIntervalAlloc) {}

  void AddLoop(size_t StartIdx, size_t EndIdx,
               const Loop& LP) {
    Loops.insert(StartIdx, EndIdx, LP);
  }

  using loop_iterator
    = typename decltype(Loops)::iterator;
  using const_loop_iterator
    = typename decltype(Loops)::const_iterator;

  loop_iterator loop_begin() {
    return Loops.begin();
  }
  const_loop_iterator loop_begin() const {
    return Loops.begin();
  }

  loop_iterator loop_end() {
    return Loops.end();
  }
  const_loop_iterator loop_end() const {
    return Loops.end();
  }
};
} // end namespace clang
#endif
