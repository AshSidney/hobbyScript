#include <CppScript/Parser.h>
#include <gmock/gmock.h>
#include "TestUtils.h"

using namespace CppScript;
using namespace CppScript::Grammar;

template <typename R>
BuildContext parse(std::string_view source)
{
   BuildContext context;
   peg::string_input in( source, "" );
   peg::parse<R, BuildAction>( in, context );
   return context;
}


TEST(ParserTest, Assignment)
{
   auto result = parse<Assignment>("ab=123");
   EXPECT_EQ(result.operationBlock.operations.size(), 1);
   EXPECT_EQ(result.operationBlock.operations[0].operationId, Id{"="});
   EXPECT_EQ(result.operationBlock.operations[0].argumentPlaces.size(), 2);
   EXPECT_EQ(result.operationBlock.operations[0].argumentPlaces[0], (ValuePlace{ ValuePlace::Type::Constants, 0 }));
   EXPECT_EQ(result.operationBlock.operations[0].argumentPlaces[1], (ValuePlace{ ValuePlace::Type::Local, 0 }));
   EXPECT_EQ(result.constantValues.size(), 1);
   EXPECT_EQ(static_cast<Value<const IntValue>&>(*result.constantValues[0]).get(), IntValue{"123"});
   EXPECT_EQ(result.variablePlaces.size(), 1);
}





namespace pegtl = tao::pegtl;

namespace CppScriptTest
{
   // Parsing rule that matches a literal "Hello, ".

   struct prefix
      : pegtl::string< 'H', 'e', 'l', 'l', 'o', ',', ' ' >
   {};

   // Parsing rule that matches a non-empty sequence of
   // alphabetic ascii-characters with greedy-matching.

   struct name
      : pegtl::plus< pegtl::alpha >
   {};

   // Parsing rule that matches a sequence of the 'prefix'
   // rule, the 'name' rule, a literal "!", and 'eof'
   // (end-of-file/input), and that throws an exception
   // on failure.

   struct grammar
      : pegtl::must< prefix, name, pegtl::one< '!' >, pegtl::eof >
   {};

   // Class template for user-defined actions that does
   // nothing by default.

   template< typename Rule >
   struct action
   {};

   // Specialisation of the user-defined action to do
   // something when the 'name' rule succeeds; is called
   // with the portion of the input that matched the rule.

   template<>
   struct action< name >
   {
      template< typename ParseInput >
      static void apply( const ParseInput& in, std::string& v )
      {
         v = in.string();
      }
   };

}

TEST(ParserTest, Prototype)
{
   std::string name;

   pegtl::string_input in( "Hello, pokus!", "" );
   pegtl::parse< CppScriptTest::grammar, CppScriptTest::action >( in, name );

   EXPECT_EQ(name, "pokus");
}