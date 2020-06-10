#include "NacroParsers.h"
#include "NacroExpanders.h"
#include "LexingTestFixture.h"
#include <memory>
#include <utility>

using namespace clang;

class NacroExpanderTest : public NacroLexingTest {
protected:
  NacroExpanderTest() = default;

  std::pair<NacroRule*, std::unique_ptr<Preprocessor>>
  GetRuleEssential(StringRef Source) {
    TrivialModuleLoader ModLoader;
    auto PP = CreatePP(Source, ModLoader);
    NacroRuleParser Parser(*PP, {});
    // Assume parser is correct...
    Parser.Parse();

    return std::make_pair(Parser.getNacroRule(),
                          std::move(PP));
  }
};

TEST_F(NacroExpanderTest, TestRuleReplacementExprProtecting) {
  auto RE = GetRuleEssential("(a:$expr, b:$expr)-> {puts(a); puts(b);}");
  auto& Rule = *RE.first;
  auto& PP = *RE.second;

  auto PrevTokenSize = Rule.token_size();
  NacroRuleExpander Expander(std::move(RE.first), PP);
  auto E = Expander.ReplacementProtecting();
  ASSERT_FALSE(E);

  ASSERT_EQ(Expander.getNacroRule()->token_size(), PrevTokenSize + 4);
}

TEST_F(NacroExpanderTest, TestRuleReplacementStringifyNoProtect) {
  // Don't protect $expr replacement if it's gonna be stringified
  auto RE = GetRuleEssential("(a:$expr, b:$expr)-> {puts($str(a)); puts(b);}");
  auto& Rule = *RE.first;
  auto& PP = *RE.second;

  NacroRuleExpander Expander(std::move(RE.first), PP);
  auto E = Expander.ReplacementProtecting();
  ASSERT_FALSE(E);

  unsigned IdxA = 0, IdxB = 0;
  for(int I = 0, E = Rule.token_size(); I < E; ++I) {
    auto Tok = Rule.getToken(I);
    if(Tok.is(tok::identifier)) {
      if(Tok.getIdentifierInfo()->isStr("a"))
        IdxA = I;
      if(Tok.getIdentifierInfo()->isStr("b"))
        IdxB = I;
    }
  }
  ASSERT_GT(IdxA, 0);
  ASSERT_GT(IdxB, 0);

  ASSERT_TRUE(Rule.getToken(IdxA - 1).is(tok::hash));

  ASSERT_TRUE(Rule.getToken(IdxB - 1).is(tok::l_paren));
  ASSERT_TRUE(Rule.getToken(IdxB + 1).is(tok::r_paren));
}
