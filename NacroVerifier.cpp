#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "NacroVerifier.h"

using namespace clang;

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
    : Ctx(Context) {}

private:
  ASTContext& Ctx;
};

struct NacroVerifierImpl : public ASTConsumer {
  explicit NacroVerifierImpl(ASTContext& Ctx)
    : DeclRefChecker(Ctx) {}

  void HandleTranslationUnit(ASTContext& Ctx) override {
    // TODO
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
FrontendPluginRegistry::Add<NacroVerifierImplAction> X("nacro-verifier",
                                                       "Nacro verifier driver");
