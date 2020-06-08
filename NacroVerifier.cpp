#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/ADT/IntervalMap.h"
#include "llvm/Support/raw_ostream.h"
#include "NacroVerifier.h"
#include <vector>

using namespace clang;

namespace llvm {
template<>
struct IntervalMapHalfOpenInfo<FullSourceLoc> {
  static inline bool isUnfairComparison(const FullSourceLoc& LHS,
                                        const FullSourceLoc& RHS) {
    return (!LHS.hasManager() || !RHS.hasManager()) ||
           (&LHS.getManager() != &RHS.getManager());
  }

  /// startLess - Return true if x is not in [a;b).
  static inline
  bool startLess(const FullSourceLoc &x, const FullSourceLoc &a) {
    if(isUnfairComparison(x, a)) {
      return x.getRawEncoding() < a.getRawEncoding();
    } else {
      auto& SM = x.getManager();
      return SM.isBeforeInTranslationUnit(cast<const SourceLocation>(x),
                                          cast<const SourceLocation>(a));
    }
  }

  /// stopLess - Return true if x is not in [a;b).
  static inline
  bool stopLess(const FullSourceLoc &b, const FullSourceLoc &x) {
    if(isUnfairComparison(b, x)) {
      return b.getRawEncoding() <= x.getRawEncoding();
    } else {
      auto& SM = x.getManager();
      return SM.isBeforeInTranslationUnit(cast<const SourceLocation>(b),
                                          cast<const SourceLocation>(x)) ||
             b == x;
    }
  }

  /// adjacent - Return true when the intervals [x;a) and [b;y) can coalesce.
  static inline
  bool adjacent(const FullSourceLoc &a, const FullSourceLoc &b) {
    return a == b;
  }

  /// nonEmpty - Return true if [a;b) is non-empty.
  static inline
  bool nonEmpty(const FullSourceLoc &a, const FullSourceLoc &b) {
    if(isUnfairComparison(a, b)) {
      return a.getRawEncoding() < b.getRawEncoding();
    } else {
      auto& SM = a.getManager();
      return SM.isBeforeInTranslationUnit(cast<const SourceLocation>(a),
                                          cast<const SourceLocation>(b));
    }
  }
};
} // end namespace clang

namespace {
struct NacroRuleDepot {
  using IntervalTy
    = llvm::IntervalMap<FullSourceLoc, NacroRule*,
          llvm::IntervalMapImpl::NodeSizer<FullSourceLoc, NacroRule*>::LeafSize,
          llvm::IntervalMapHalfOpenInfo<FullSourceLoc>>;
  typename IntervalTy::Allocator Allocator;
  IntervalTy Intervals;

  NacroRuleDepot()
    : Allocator(),
      Intervals(Allocator) {}

  inline
  operator bool() const {
    return !Intervals.empty();
  }
};
} // end anonymous namespace

static NacroRuleDepot NacroRules;

void NacroVerifier::AddNacroRule(NacroRule* Rule) {
  auto SR = Rule->getSourceRange();
  auto B = SR.getBegin(), E = SR.getEnd();
  NacroRules.Intervals.insert(FullSourceLoc(B, SM), FullSourceLoc(E, SM),
                              Rule);
}

namespace {
/// Try to warn the following use case
/// ```
/// #define hello(arg) { \
///   int x = 0; \
///   return arg + x - 87; \
/// }
/// int foo(int x) {
///   hello(x)
/// }
/// ```
struct NacroDeclRefChecker
  : public RecursiveASTVisitor<NacroDeclRefChecker> {
  explicit NacroDeclRefChecker(ASTContext& Context)
    : Ctx(Context),
      SM(Ctx.getSourceManager()) {}

  bool VisitDeclRefExpr(DeclRefExpr* DRE) {
    if(!NacroRules) return true;

    auto DRELoc
      = FullSourceLoc(SM.getSpellingLoc(DRE->getLocation()), SM);
    auto* D = DRE->getDecl();
    auto DLoc
      = FullSourceLoc(SM.getSpellingLoc(D->getLocation()), SM);
    // Throw a warming if one of DRELoc or DLoc is in
    // a nacro but the other is not
    auto* RefR = NacroRules.Intervals.lookup(DRELoc);
    auto* DeclR = NacroRules.Intervals.lookup(DLoc);
    if(RefR != DeclR &&
       DeclR != nullptr) {
      llvm::errs() << "Warning: A potential declaration leaking happens\n";
    }
    return true;
  }

private:
  ASTContext& Ctx;
  SourceManager& SM;
};

struct NacroVerifierImpl : public ASTConsumer {
  explicit NacroVerifierImpl(ASTContext& Ctx)
    : DeclRefChecker(Ctx) {}

  void HandleTranslationUnit(ASTContext& Ctx) override {
    DeclRefChecker.TraverseAST(Ctx);
  }

private:
  NacroDeclRefChecker DeclRefChecker;
};

struct NacroVerifierImplAction : public PluginASTAction {
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) override {
    return std::unique_ptr<clang::ASTConsumer>(
      new NacroVerifierImpl(Compiler.getASTContext()));
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string>& args) override {
    return true;
  }

  ActionType getActionType() override {
    return PluginASTAction::AddBeforeMainAction;
  }
};
} // end anonymous namespace

static
FrontendPluginRegistry::Add<NacroVerifierImplAction>
  X("nacro-verifier", "Nacro verifier driver");
