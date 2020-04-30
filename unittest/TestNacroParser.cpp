#include "NacroParsers.h"
#include "LexingTestFixture.h"

using namespace clang;

class NacroParserTest : public NacroLexingTest {
protected:
  NacroParserTest() = default;

  // Simple wrapper around CreatePP
  std::unique_ptr<Preprocessor> GetPP(StringRef Source) {
    TrivialModuleLoader ModLoader;
    return CreatePP(Source, ModLoader);
  }
};

TEST_F(NacroParserTest, TestRuleParseArgList) {
  auto PP = GetPP("a:$expr, b:$stmt)");
  NacroRuleParser Parser(*PP, {});

  ASSERT_TRUE(Parser.ParseArgList());
  auto& Rule = Parser.getNacroRule();
  ASSERT_EQ(Rule.replacements_size(), 2);

  auto RI = Rule.replacement_begin();
  ASSERT_EQ(RI->Type, NacroRule::ReplacementTy::Expr);
  ++RI;
  ASSERT_EQ(RI->Type, NacroRule::ReplacementTy::Stmt);
}
