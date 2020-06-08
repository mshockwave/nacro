#ifndef NACRO_NACRO_VERIFIER_H
#define NACRO_NACRO_VERIFIER_H
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Preprocessor.h"
#include "NacroRule.h"
#include <memory>

namespace clang {
/// Only used as a frontend to receive NacroRule information
/// (espcially line range info) from previous stages in the pipeline.
struct NacroVerifier {
  NacroVerifier(SourceManager& SM)
    : SM(SM) {}

  void AddNacroRule(NacroRule* Rule);

private:
  SourceManager& SM;
};
} // end namespace clang
#endif
