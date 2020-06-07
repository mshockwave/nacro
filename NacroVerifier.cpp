#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/Support/raw_ostream.h"
#include "NacroVerifier.h"
#include <vector>

using namespace clang;

static std::vector<std::unique_ptr<NacroRule>> NacroRules;

void NacroVerifier::AddNacroRule(std::unique_ptr<NacroRule>&& Rule) {
  NacroRules.emplace_back(std::move(Rule));
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
    auto DRELoc = SM.getSpellingLoc(DRE->getLocation());
    auto* D = DRE->getDecl();
    auto DLoc = SM.getSpellingLoc(D->getLocation());
    // TODO: Throw a warming if one of DRELoc or DLoc is in
    // a nacro but the other is not
    (void)DRELoc;
    (void)DLoc;
    llvm::errs() << NacroRules.size() << "\n";
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
