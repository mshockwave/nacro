#include "clang/AST/ASTConsumer.h"
#include "clang/AST/Expr.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;

class FindNamedClassVisitor
  : public RecursiveASTVisitor<FindNamedClassVisitor> {
public:
  explicit FindNamedClassVisitor(ASTContext *Context)
    : Ctx(Context) {}

  bool VisitCXXRecordDecl(CXXRecordDecl *Declaration) {
#if 0
    if (Declaration->getQualifiedNameAsString() == "n::m::C") {
      FullSourceLoc FullLocation = Context->getFullLoc(Declaration->getBeginLoc());
      if (FullLocation.isValid())
        llvm::outs() << "Found declaration at "
                     << FullLocation.getSpellingLineNumber() << ":"
                     << FullLocation.getSpellingColumnNumber() << "\n";
    }
#endif
    return true;
  }

  template<class T>
  void printLocation(T* Obj) {
    auto& SM = Ctx->getSourceManager();
    auto Loc = Obj->getLocation();
    llvm::errs() << "#Loc: "
                 << Loc.printToString(SM) << "\n";
    Loc = Obj->getBeginLoc();
    auto ELoc = Obj->getEndLoc();
    llvm::errs() << "#Begin/End: "
                 << Loc.printToString(SM) << " / "
                 << ELoc.printToString(SM) << "\n\n";
  }

  bool VisitDeclRefExpr(DeclRefExpr* Ref) {
    llvm::errs() << "In DeclRefExpr...\n";
    printLocation<DeclRefExpr>(Ref);
    printLocation<Decl>(Ref->getDecl());
    return true;
  }

private:
  ASTContext *Ctx;
};

class FindNamedClassConsumer : public clang::ASTConsumer {
public:
  explicit FindNamedClassConsumer(ASTContext *Context)
    : Visitor(Context) {}

  virtual void HandleTranslationUnit(clang::ASTContext &Context) {
    Visitor.TraverseDecl(Context.getTranslationUnitDecl());
  }
private:
  FindNamedClassVisitor Visitor;
};

struct FindNamedClassAction : public clang::PluginASTAction {
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance &Compiler, llvm::StringRef InFile) override {
    return std::unique_ptr<clang::ASTConsumer>(
        new FindNamedClassConsumer(&Compiler.getASTContext()));
  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string>& args) override {
    return true;
  }
};

static
FrontendPluginRegistry::Add<FindNamedClassAction> X("example-plugin",
                                                    "my plugin description");
