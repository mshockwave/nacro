#ifndef NACRO_NACRO_VERIFIER_H
#define NACRO_NACRO_VERIFIER_H
#include "clang/Lex/Preprocessor.h"
#include "NacroRule.h"
#include <memory>

namespace clang {
/// Only used as a frontend to receive NacroRule information
/// (espcially line range info) from previous stages in the pipeline.
struct NacroVerifier {
  void AddNacroRule(std::unique_ptr<NacroRule>&& Rule);
};
} // end namespace clang
#endif
