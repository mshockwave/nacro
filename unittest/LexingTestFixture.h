#ifndef NACRO_UNITTEST_LEXINGTESTFIXTURE_H
#define NACRO_UNITTEST_LEXINGTESTFIXTURE_H
#include "clang/Lex/Lexer.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/SourceLocation.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TokenKinds.h"
#include "clang/Lex/HeaderSearch.h"
#include "clang/Lex/HeaderSearchOptions.h"
#include "clang/Lex/MacroArgs.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/ModuleLoader.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Lex/PreprocessorOptions.h"
#include "gtest/gtest.h"

namespace clang {

// Test fixture for nacro tests related to lexing.
// (i.e. PP plugins, Nacro DSL parsing etc.)
// Copied from clang's unittest framework
class NacroLexingTest : public ::testing::Test {
protected:
  NacroLexingTest()
    : FileMgr(FileMgrOpts),
      DiagID(new DiagnosticIDs()),
      Diags(DiagID, new DiagnosticOptions, new IgnoringDiagConsumer()),
      SourceMgr(Diags, FileMgr),
      TargetOpts(new TargetOptions)
  {
    TargetOpts->Triple = "x86_64-apple-darwin11.1.0";
    Target = TargetInfo::CreateTargetInfo(Diags, TargetOpts);
  }

  std::unique_ptr<Preprocessor> CreatePP(StringRef Source,
                                         TrivialModuleLoader &ModLoader) {
    std::unique_ptr<llvm::MemoryBuffer> Buf =
        llvm::MemoryBuffer::getMemBuffer(Source);
    SourceMgr.setMainFileID(SourceMgr.createFileID(std::move(Buf)));

    HeaderSearch HeaderInfo(std::make_shared<HeaderSearchOptions>(), SourceMgr,
                            Diags, LangOpts, Target.get());
    std::unique_ptr<Preprocessor> PP = std::make_unique<Preprocessor>(
        std::make_shared<PreprocessorOptions>(), Diags, LangOpts, SourceMgr,
        HeaderInfo, ModLoader,
        /*IILookup =*/nullptr,
        /*OwnsHeaderSearch =*/false);
    PP->Initialize(*Target);
    PP->EnterMainSourceFile();
    return PP;
  }

  std::vector<Token> Lex(StringRef Source) {
    TrivialModuleLoader ModLoader;
    auto PP = CreatePP(Source, ModLoader);

    std::vector<Token> toks;
    while (1) {
      Token tok;
      PP->Lex(tok);
      if (tok.is(tok::eof))
        break;
      toks.push_back(tok);
    }

    return toks;
  }

  std::vector<Token> CheckLex(StringRef Source,
                              ArrayRef<tok::TokenKind> ExpectedTokens) {
    auto toks = Lex(Source);
    EXPECT_EQ(ExpectedTokens.size(), toks.size());
    for (unsigned i = 0, e = ExpectedTokens.size(); i != e; ++i) {
      EXPECT_EQ(ExpectedTokens[i], toks[i].getKind());
    }

    return toks;
  }

  std::string getSourceText(Token Begin, Token End) {
    bool Invalid;
    StringRef Str =
        Lexer::getSourceText(CharSourceRange::getTokenRange(SourceRange(
                                    Begin.getLocation(), End.getLocation())),
                             SourceMgr, LangOpts, &Invalid);
    if (Invalid)
      return "<INVALID>";
    return std::string(Str);
  }

  FileSystemOptions FileMgrOpts;
  FileManager FileMgr;
  IntrusiveRefCntPtr<DiagnosticIDs> DiagID;
  DiagnosticsEngine Diags;
  SourceManager SourceMgr;
  LangOptions LangOpts;
  std::shared_ptr<TargetOptions> TargetOpts;
  IntrusiveRefCntPtr<TargetInfo> Target;
};

} // end namespace clang
#endif
