#ifndef NACRO_NACRO_RULE_H
#define NACRO_NACRO_RULE_H
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/IntervalMap.h"
#include "llvm/ADT/iterator_range.h"
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

  /// null if name is not set
  IdentifierInfo* getName() const { return Name; }

  ReplacementTy getGeneratedType() const { return GeneratedType; }
  void setGeneratedType(ReplacementTy RT) {
    GeneratedType = RT;
  }

  void setSourceRange(SourceRange SR) {
    SrcRange = SR;
  }

  SourceLocation getBeginLoc() const { return SrcRange.getBegin(); }

  SourceRange getSourceRange() const {
    return SrcRange;
  }

private:
  IdentifierInfo* Name;

  /// Range of this rule (starts from the head of rule
  /// rather than pragma)
  SourceRange SrcRange;

  /// a.k.a Macro rule arguments
  llvm::SmallVector<Replacement, 2> Replacements;

  /// a.k.a 'return type'
  ReplacementTy GeneratedType;

  /// Note that a loop region is surrounded by a pair of
  /// tok::annot_pragma_loop_hint
  llvm::SmallVector<Token, 16> Tokens;

  llvm::SmallVector<Loop, 2> Loops;

  NacroRule(IdentifierInfo* NameII)
    : Name(NameII), SrcRange(),
      GeneratedType(ReplacementTy::Block) {}

public:
  static NacroRule* Create(IdentifierInfo* NameII);

  using repl_iterator
    = typename decltype(Replacements)::iterator;

  repl_iterator replacement_begin() {
    return Replacements.begin();
  }

  repl_iterator replacement_end() {
    return Replacements.end();
  }

  auto replacements() {
    return llvm::make_range(replacement_begin(),
                            replacement_end());
  }

  size_t replacements_size() const {
    return Replacements.size();
  }

  inline
  Replacement& getReplacement(size_t i) {
    assert(i < replacements_size());
    return Replacements[i];
  }

  inline
  const Replacement& getReplacement(size_t i) const {
    assert(i < replacements_size());
    return Replacements[i];
  }

  inline bool hasVAArgs() const {
    return llvm::any_of(Replacements,
                        [](const Replacement& R) -> bool {
                          return R.VarArgs;
                        });
  }

  using token_iterator =
    typename decltype(Tokens)::iterator;

  token_iterator token_begin() {
    return Tokens.begin();
  }

  token_iterator token_end() {
    return Tokens.end();
  }

  Token token_front() const {
    return Tokens.front();
  }

  Token token_back() const {
    return Tokens.back();
  }

  size_t token_size() const { return Tokens.size(); }

  auto tokens() {
    return llvm::make_range(token_begin(), token_end());
  }

  inline
  Token getToken(size_t i) const {
    assert(i < Tokens.size());
    return Tokens[i];
  }

  token_iterator insert_token(token_iterator pos, const Token& tok) {
    return Tokens.insert(pos, tok);
  }

  token_iterator erase_token(token_iterator pos) {
    return Tokens.erase(pos);
  }

  inline void AddLoop(const Loop& LP) {
    Loops.push_back(LP);
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

  bool loop_empty() const {
    return Loops.empty();
  }

  inline
  Loop& getLoop(size_t Idx) {
    return Loops[Idx];
  }
  inline
  const Loop& getLoop(size_t Idx) const {
    return Loops[Idx];
  }

  /// Require installing PPCallbacks (e.g. loops)
  bool needsPPHooks() const;
};
} // end namespace clang
#endif
