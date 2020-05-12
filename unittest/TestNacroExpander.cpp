#include "NacroParsers.h"
#include "NacroExpander.h"
#include "LexingTestFixture.h"
#include <memory>
#include <utility>

using namespace clang;

class NacroExpanderTest : public NacroLexingTest {
protected:
  NacroExpanderTest() = default;

  std::pair<std::unique_ptr<NacroRule>, std::unique_ptr<Preprocessor>>
  GetRuleEssential(StringRef Source) {
    TrivialModuleLoader ModLoader;
    auto PP = CreatePP(Source, ModLoader);
    NacroRuleParser Parser(*PP, {});
    // Assume parser is correct...
    Parser.Parse();

    return std::make_pair(std::move(Parser.releaseNacroRule()),
                          std::move(PP));
  }
};

TEST_F(NacroExpanderTest, TestRuleReplacementExprProtecting) {
  auto RE = GetRuleEssential("(a:$expr, b:$expr)-> {puts(a); puts(b);}");
  auto& Rule = *RE.first;
  auto& PP = *RE.second;

  auto PrevTokenSize = Rule.token_size();
  NacroRuleExpander Expander(Rule, PP);
  auto E = Expander.ReplacementProtecting();
  ASSERT_FALSE(E);

  ASSERT_EQ(Rule.token_size(), PrevTokenSize + 4);
}
