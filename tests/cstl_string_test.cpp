#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

extern "C"
{
#include "rlgame.base/cstl/cstl_string.h"
#include "rlgame.base/cstl/cstl_heap_allocator.h"
}

namespace
{

constexpr size_t kTestHeapSize = 256 * 1024;

class CstlStringTest : public ::testing::Test
{
  protected:
    void
    SetUp () override
    { ASSERT_EQ (0, R_CSTL_HeapInit (kTestHeapSize)); }

    void
    TearDown () override
    { R_CSTL_HeapShutdown (); }

    static void
    ExpectStringData (const struct R_CSTL_String* pString, const std::string& expected)
    {
      ASSERT_NE (nullptr, pString);
      EXPECT_EQ (expected.size (), R_CSTL_StringLength (pString));
      const char* pData = R_CSTL_StringData (pString);
      ASSERT_NE (nullptr, pData);
      EXPECT_EQ (expected, std::string (pData, R_CSTL_StringLength (pString)));
    }
};

} // namespace

TEST (CstlStringInitTest, DeleteNullIsSafe)
{
  R_CSTL_StringDelete (nullptr);
  SUCCEED ();
}

TEST_F (CstlStringTest, NewEmptyString)
{
  struct R_CSTL_String* pString = R_CSTL_NewString ();
  ASSERT_NE (nullptr, pString);
  EXPECT_EQ (0u, R_CSTL_StringLength (pString));
  ExpectStringData (pString, "");
  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, NewStringWithData)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("Hello");
  ASSERT_NE (nullptr, pString);
  ExpectStringData (pString, "Hello");
  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, NewStringWithDataSized)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithDataSized ("HelloWorld", 5);
  ASSERT_NE (nullptr, pString);
  ExpectStringData (pString, "Hello");
  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, NewStringWithDataRejectsNull)
{
  EXPECT_EQ (nullptr, R_CSTL_NewStringWithData (nullptr));
}

TEST_F (CstlStringTest, NewStringWithFormat)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithFormat ("%s %d", "Test", 42);
  ASSERT_NE (nullptr, pString);
  ExpectStringData (pString, "Test 42");
  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, NewStringWithFormatNullReturnsEmpty)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithFormat (nullptr);
  ASSERT_NE (nullptr, pString);
  ExpectStringData (pString, "");
  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, CharAtBoundsCheck)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("ABC");
  ASSERT_NE (nullptr, pString);

  EXPECT_EQ ('A', R_CSTL_StringCharAt (pString, 0));
  EXPECT_EQ ('B', R_CSTL_StringCharAt (pString, 1));
  EXPECT_EQ ('C', R_CSTL_StringCharAt (pString, 2));
  EXPECT_EQ (0x00, R_CSTL_StringCharAt (pString, 3)); // Out of bounds
  EXPECT_EQ (0x00, R_CSTL_StringCharAt (nullptr, 0));

  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, StringEquals)
{
  struct R_CSTL_String* pString1 = R_CSTL_NewStringWithData ("Hello");
  struct R_CSTL_String* pString2 = R_CSTL_NewStringWithData ("Hello");
  struct R_CSTL_String* pString3 = R_CSTL_NewStringWithData ("World");

  ASSERT_NE (nullptr, pString1);
  ASSERT_NE (nullptr, pString2);
  ASSERT_NE (nullptr, pString3);

  EXPECT_EQ (1, R_CSTL_StringEquals (pString1, pString2));
  EXPECT_EQ (0, R_CSTL_StringEquals (pString1, pString3));
  EXPECT_EQ (1, R_CSTL_StringEquals (pString1, pString1));
  EXPECT_EQ (0, R_CSTL_StringEquals (pString1, nullptr));

  R_CSTL_StringDelete (pString1);
  R_CSTL_StringDelete (pString2);
  R_CSTL_StringDelete (pString3);
}

TEST_F (CstlStringTest, StringCompare)
{
  struct R_CSTL_String* pString1 = R_CSTL_NewStringWithData ("Apple");
  struct R_CSTL_String* pString2 = R_CSTL_NewStringWithData ("Banana");
  struct R_CSTL_String* pString3 = R_CSTL_NewStringWithData ("Apple");

  ASSERT_NE (nullptr, pString1);
  ASSERT_NE (nullptr, pString2);
  ASSERT_NE (nullptr, pString3);

  EXPECT_LT (R_CSTL_StringCompare (pString1, pString2), 0);
  EXPECT_GT (R_CSTL_StringCompare (pString2, pString1), 0);
  EXPECT_EQ (0, R_CSTL_StringCompare (pString1, pString3));
  EXPECT_LT (R_CSTL_StringCompare (nullptr, pString1), 0);
  EXPECT_GT (R_CSTL_StringCompare (pString1, nullptr), 0);

  R_CSTL_StringDelete (pString1);
  R_CSTL_StringDelete (pString2);
  R_CSTL_StringDelete (pString3);
}

TEST_F (CstlStringTest, StringConcat)
{
  struct R_CSTL_String* pString1 = R_CSTL_NewStringWithData ("Hello");
  struct R_CSTL_String* pString2 = R_CSTL_NewStringWithData (" World");

  ASSERT_NE (nullptr, pString1);
  ASSERT_NE (nullptr, pString2);

  struct R_CSTL_String* pResult = R_CSTL_StringConcat (pString1, pString2);
  ASSERT_NE (nullptr, pResult);
  ExpectStringData (pResult, "Hello World");

  R_CSTL_StringDelete (pString1);
  R_CSTL_StringDelete (pString2);
  R_CSTL_StringDelete (pResult);
}

TEST_F (CstlStringTest, StringConcatWithNull)
{
  struct R_CSTL_String* pString1 = R_CSTL_NewStringWithData ("Hello");
  ASSERT_NE (nullptr, pString1);

  struct R_CSTL_String* pResult1 = R_CSTL_StringConcat (pString1, nullptr);
  ASSERT_NE (nullptr, pResult1);
  ExpectStringData (pResult1, "Hello");

  struct R_CSTL_String* pResult2 = R_CSTL_StringConcat (nullptr, pString1);
  ASSERT_NE (nullptr, pResult2);
  ExpectStringData (pResult2, "Hello");

  struct R_CSTL_String* pResult3 = R_CSTL_StringConcat (nullptr, nullptr);
  ASSERT_NE (nullptr, pResult3);
  ExpectStringData (pResult3, "");

  R_CSTL_StringDelete (pString1);
  R_CSTL_StringDelete (pResult1);
  R_CSTL_StringDelete (pResult2);
  R_CSTL_StringDelete (pResult3);
}

TEST_F (CstlStringTest, StringSubstring)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("HelloWorld");
  ASSERT_NE (nullptr, pString);

  struct R_CSTL_String* pSub1 = R_CSTL_StringSubstring (pString, 0, 5);
  ASSERT_NE (nullptr, pSub1);
  ExpectStringData (pSub1, "Hello");

  struct R_CSTL_String* pSub2 = R_CSTL_StringSubstring (pString, 5, 10);
  ASSERT_NE (nullptr, pSub2);
  ExpectStringData (pSub2, "World");

  struct R_CSTL_String* pSub3 = R_CSTL_StringSubstring (pString, 3, 7);
  ASSERT_NE (nullptr, pSub3);
  ExpectStringData (pSub3, "loWo");

  R_CSTL_StringDelete (pString);
  R_CSTL_StringDelete (pSub1);
  R_CSTL_StringDelete (pSub2);
  R_CSTL_StringDelete (pSub3);
}

TEST_F (CstlStringTest, StringSubstringInvalidRange)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("Hello");
  ASSERT_NE (nullptr, pString);

  struct R_CSTL_String* pSub1 = R_CSTL_StringSubstring (pString, 10, 15);
  ASSERT_NE (nullptr, pSub1);
  ExpectStringData (pSub1, "");

  struct R_CSTL_String* pSub2 = R_CSTL_StringSubstring (pString, 3, 3);
  ASSERT_NE (nullptr, pSub2);
  ExpectStringData (pSub2, "");

  EXPECT_EQ (nullptr, R_CSTL_StringSubstring (nullptr, 0, 5));

  R_CSTL_StringDelete (pString);
  R_CSTL_StringDelete (pSub1);
  R_CSTL_StringDelete (pSub2);
}

TEST_F (CstlStringTest, StringStartsWith)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("HelloWorld");
  ASSERT_NE (nullptr, pString);

  EXPECT_EQ (1, R_CSTL_StringStartsWith (pString, "Hello"));
  EXPECT_EQ (0, R_CSTL_StringStartsWith (pString, "World"));
  EXPECT_EQ (1, R_CSTL_StringStartsWith (pString, ""));
  EXPECT_EQ (0, R_CSTL_StringStartsWith (pString, "HelloWorld!"));
  EXPECT_EQ (0, R_CSTL_StringStartsWith (nullptr, "Hello"));
  EXPECT_EQ (0, R_CSTL_StringStartsWith (pString, nullptr));

  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, StringEndsWith)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("HelloWorld");
  ASSERT_NE (nullptr, pString);

  EXPECT_EQ (1, R_CSTL_StringEndsWith (pString, "World"));
  EXPECT_EQ (0, R_CSTL_StringEndsWith (pString, "Hello"));
  EXPECT_EQ (1, R_CSTL_StringEndsWith (pString, ""));
  EXPECT_EQ (0, R_CSTL_StringEndsWith (pString, "!HelloWorld"));
  EXPECT_EQ (0, R_CSTL_StringEndsWith (nullptr, "World"));
  EXPECT_EQ (0, R_CSTL_StringEndsWith (pString, nullptr));

  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, StringIndexOf)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("HelloWorld");
  ASSERT_NE (nullptr, pString);

  EXPECT_EQ (0u, R_CSTL_StringIndexOf (pString, "Hello"));
  EXPECT_EQ (5u, R_CSTL_StringIndexOf (pString, "World"));
  EXPECT_EQ (2u, R_CSTL_StringIndexOf (pString, "llo"));
  EXPECT_EQ ((size_t)-1, R_CSTL_StringIndexOf (pString, "xyz"));
  EXPECT_EQ ((size_t)-1, R_CSTL_StringIndexOf (nullptr, "Hello"));
  EXPECT_EQ ((size_t)-1, R_CSTL_StringIndexOf (pString, nullptr));

  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, StringLastIndexOf)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("HelloHello");
  ASSERT_NE (nullptr, pString);

  EXPECT_EQ (5u, R_CSTL_StringLastIndexOf (pString, "Hello"));
  EXPECT_EQ (7u, R_CSTL_StringLastIndexOf (pString, "llo"));
  EXPECT_EQ ((size_t)-1, R_CSTL_StringLastIndexOf (pString, "xyz"));
  EXPECT_EQ ((size_t)-1, R_CSTL_StringLastIndexOf (nullptr, "Hello"));
  EXPECT_EQ ((size_t)-1, R_CSTL_StringLastIndexOf (pString, nullptr));

  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, StringIndexOfChar)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("HelloWorld");
  ASSERT_NE (nullptr, pString);

  EXPECT_EQ (2u, R_CSTL_StringIndexOfChar (pString, 'l'));
  EXPECT_EQ (0u, R_CSTL_StringIndexOfChar (pString, 'H'));
  EXPECT_EQ ((size_t)-1, R_CSTL_StringIndexOfChar (pString, 'z'));
  EXPECT_EQ ((size_t)-1, R_CSTL_StringIndexOfChar (nullptr, 'H'));

  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, StringLastIndexOfChar)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("HelloWorld");
  ASSERT_NE (nullptr, pString);

  EXPECT_EQ (8u, R_CSTL_StringLastIndexOfChar (pString, 'l'));
  EXPECT_EQ (5u, R_CSTL_StringLastIndexOfChar (pString, 'W'));
  EXPECT_EQ ((size_t)-1, R_CSTL_StringLastIndexOfChar (pString, 'z'));
  EXPECT_EQ ((size_t)-1, R_CSTL_StringLastIndexOfChar (nullptr, 'H'));

  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, StringContains)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("HelloWorld");
  ASSERT_NE (nullptr, pString);

  EXPECT_EQ (1, R_CSTL_StringContains (pString, "Hello"));
  EXPECT_EQ (0, R_CSTL_StringContains (pString, "xyz"));

  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, StringIsEmpty)
{
  struct R_CSTL_String* pString1 = R_CSTL_NewStringWithData ("");
  struct R_CSTL_String* pString2 = R_CSTL_NewStringWithData ("Hello");

  ASSERT_NE (nullptr, pString1);
  ASSERT_NE (nullptr, pString2);

  EXPECT_EQ (1, R_CSTL_StringIsEmpty (pString1));
  EXPECT_EQ (0, R_CSTL_StringIsEmpty (pString2));
  EXPECT_EQ (-1, R_CSTL_StringIsEmpty (nullptr));

  R_CSTL_StringDelete (pString1);
  R_CSTL_StringDelete (pString2);
}

TEST_F (CstlStringTest, StringToLowerCase)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("HeLLo WoRLd");
  ASSERT_NE (nullptr, pString);

  struct R_CSTL_String* pResult = R_CSTL_StringToLowerCase (pString);
  ASSERT_NE (nullptr, pResult);
  ExpectStringData (pResult, "hello world");

  R_CSTL_StringDelete (pString);
  R_CSTL_StringDelete (pResult);
}

TEST_F (CstlStringTest, StringToUpperCase)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("HeLLo WoRLd");
  ASSERT_NE (nullptr, pString);

  struct R_CSTL_String* pResult = R_CSTL_StringToUpperCase (pString);
  ASSERT_NE (nullptr, pResult);
  ExpectStringData (pResult, "HELLO WORLD");

  R_CSTL_StringDelete (pString);
  R_CSTL_StringDelete (pResult);
}

TEST_F (CstlStringTest, StringTrim)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("  Hello World  ");
  ASSERT_NE (nullptr, pString);

  struct R_CSTL_String* pResult = R_CSTL_StringTrim (pString);
  ASSERT_NE (nullptr, pResult);
  ExpectStringData (pResult, "Hello World");

  R_CSTL_StringDelete (pString);
  R_CSTL_StringDelete (pResult);
}

TEST_F (CstlStringTest, StringReplace)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("Hello World");
  ASSERT_NE (nullptr, pString);

  struct R_CSTL_String* pResult = R_CSTL_StringReplace (pString, "World", "Universe");
  ASSERT_NE (nullptr, pResult);
  ExpectStringData (pResult, "Hello Universe");

  R_CSTL_StringDelete (pString);
  R_CSTL_StringDelete (pResult);
}

TEST_F (CstlStringTest, StringReplaceChar)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("Hello World");
  ASSERT_NE (nullptr, pString);

  struct R_CSTL_String* pResult = R_CSTL_StringReplaceChar (pString, 'o', 'a');
  ASSERT_NE (nullptr, pResult);
  ExpectStringData (pResult, "Hella Warld");

  R_CSTL_StringDelete (pString);
  R_CSTL_StringDelete (pResult);
}

TEST_F (CstlStringTest, StringRepeat)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("Abc");
  ASSERT_NE (nullptr, pString);

  struct R_CSTL_String* pResult = R_CSTL_StringRepeat (pString, 3);
  ASSERT_NE (nullptr, pResult);
  ExpectStringData (pResult, "AbcAbcAbc");

  R_CSTL_StringDelete (pString);
  R_CSTL_StringDelete (pResult);
}

TEST_F (CstlStringTest, StringEqualsIgnoreCase)
{
  struct R_CSTL_String* pString1 = R_CSTL_NewStringWithData ("Hello");
  struct R_CSTL_String* pString2 = R_CSTL_NewStringWithData ("HELLO");

  ASSERT_NE (nullptr, pString1);
  ASSERT_NE (nullptr, pString2);

  EXPECT_EQ (1, R_CSTL_StringEqualsIgnoreCase (pString1, pString2));
  EXPECT_EQ (0, R_CSTL_StringEqualsIgnoreCase (pString1, nullptr));

  R_CSTL_StringDelete (pString1);
  R_CSTL_StringDelete (pString2);
}

TEST_F (CstlStringTest, StringCompareIgnoreCase)
{
  struct R_CSTL_String* pString1 = R_CSTL_NewStringWithData ("apple");
  struct R_CSTL_String* pString2 = R_CSTL_NewStringWithData ("BANANA");

  ASSERT_NE (nullptr, pString1);
  ASSERT_NE (nullptr, pString2);

  EXPECT_LT (R_CSTL_StringCompareIgnoreCase (pString1, pString2), 0);
  EXPECT_EQ (0, R_CSTL_StringCompareIgnoreCase (pString1, pString1));

  R_CSTL_StringDelete (pString1);
  R_CSTL_StringDelete (pString2);
}

TEST_F (CstlStringTest, StringHashCode)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("Hello");
  ASSERT_NE (nullptr, pString);

  size_t hash1 = R_CSTL_StringHashCode (pString);
  size_t hash2 = R_CSTL_StringHashCode (pString);
  EXPECT_EQ (hash1, hash2);

  EXPECT_EQ (0u, R_CSTL_StringHashCode (nullptr));

  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, StringRemove)
{
  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("HelloWorld");
  ASSERT_NE (nullptr, pString);

  struct R_CSTL_String* pResult = R_CSTL_StringRemove (pString, 5, 10);
  ASSERT_NE (nullptr, pResult);
  ExpectStringData (pResult, "Hello");

  R_CSTL_StringDelete (pString);
  R_CSTL_StringDelete (pResult);
}

TEST_F (CstlStringTest, StringValueOfInt)
{
  struct R_CSTL_String* pString = R_CSTL_StringValueOfInt (42);
  ASSERT_NE (nullptr, pString);
  ExpectStringData (pString, "42");
  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, StringValueOfLong)
{
  struct R_CSTL_String* pString = R_CSTL_StringValueOfLong (123456789LL);
  ASSERT_NE (nullptr, pString);
  ExpectStringData (pString, "123456789");
  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, StringValueOfDouble)
{
  struct R_CSTL_String* pString = R_CSTL_StringValueOfDouble (3.14);
  ASSERT_NE (nullptr, pString);
  EXPECT_EQ (0, strncmp (R_CSTL_StringData (pString), "3.14", 4));
  R_CSTL_StringDelete (pString);
}

TEST_F (CstlStringTest, StringJoin)
{
  struct R_CSTL_String* pDelimiter = R_CSTL_NewStringWithData (", ");
  struct R_CSTL_String* pString1 = R_CSTL_NewStringWithData ("A");
  struct R_CSTL_String* pString2 = R_CSTL_NewStringWithData ("B");
  struct R_CSTL_String* pString3 = R_CSTL_NewStringWithData ("C");

  ASSERT_NE (nullptr, pDelimiter);
  ASSERT_NE (nullptr, pString1);
  ASSERT_NE (nullptr, pString2);
  ASSERT_NE (nullptr, pString3);

  const struct R_CSTL_String* strings[] = {pString1, pString2, pString3};
  struct R_CSTL_String* pResult = R_CSTL_StringJoin (pDelimiter, strings, 3);
  ASSERT_NE (nullptr, pResult);
  ExpectStringData (pResult, "A, B, C");

  R_CSTL_StringDelete (pDelimiter);
  R_CSTL_StringDelete (pString1);
  R_CSTL_StringDelete (pString2);
  R_CSTL_StringDelete (pString3);
  R_CSTL_StringDelete (pResult);
}

TEST_F (CstlStringTest, StringBuilderBasic)
{
  struct R_CSTL_StringBuilder* pBuilder = R_CSTL_NewStringBuilder ();
  ASSERT_NE (nullptr, pBuilder);
  EXPECT_EQ (0u, R_CSTL_StringBuilderLength (pBuilder));

  struct R_CSTL_String* pString = R_CSTL_NewStringWithData ("Hello");
  ASSERT_NE (nullptr, pString);
  ASSERT_EQ (0, R_CSTL_StringBuilderAppend (pBuilder, pString));

  EXPECT_EQ (5u, R_CSTL_StringBuilderLength (pBuilder));

  R_CSTL_StringBuilderClear (pBuilder);
  EXPECT_EQ (0u, R_CSTL_StringBuilderLength (pBuilder));

  R_CSTL_StringDelete (pString);
  R_CSTL_DeleteStringBuilder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderAppendData)
{
  struct R_CSTL_StringBuilder* pBuilder = R_CSTL_NewStringBuilder ();
  ASSERT_NE (nullptr, pBuilder);

  ASSERT_EQ (0, R_CSTL_StringBuilderAppendData (pBuilder, "Hello", 5));
  ASSERT_EQ (0, R_CSTL_StringBuilderAppendData (pBuilder, "World", 5));

  struct R_CSTL_String* pString = R_CSTL_StringBuilderToString (pBuilder);
  ASSERT_NE (nullptr, pString);
  ExpectStringData (pString, "HelloWorld");

  R_CSTL_StringDelete (pString);
  R_CSTL_DeleteStringBuilder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderAppendChar)
{
  struct R_CSTL_StringBuilder* pBuilder = R_CSTL_NewStringBuilder ();
  ASSERT_NE (nullptr, pBuilder);

  ASSERT_EQ (0, R_CSTL_StringBuilderAppendChar (pBuilder, 'A'));
  ASSERT_EQ (0, R_CSTL_StringBuilderAppendChar (pBuilder, 'B'));
  ASSERT_EQ (0, R_CSTL_StringBuilderAppendChar (pBuilder, 'C'));

  struct R_CSTL_String* pString = R_CSTL_StringBuilderToString (pBuilder);
  ASSERT_NE (nullptr, pString);
  ExpectStringData (pString, "ABC");

  R_CSTL_StringDelete (pString);
  R_CSTL_DeleteStringBuilder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderAppendInt)
{
  struct R_CSTL_StringBuilder* pBuilder = R_CSTL_NewStringBuilder ();
  ASSERT_NE (nullptr, pBuilder);

  ASSERT_EQ (0, R_CSTL_StringBuilderAppendInt (pBuilder, 42));
  ASSERT_EQ (0, R_CSTL_StringBuilderAppendInt (pBuilder, -123));

  struct R_CSTL_String* pString = R_CSTL_StringBuilderToString (pBuilder);
  ASSERT_NE (nullptr, pString);
  ExpectStringData (pString, "42-123");

  R_CSTL_StringDelete (pString);
  R_CSTL_DeleteStringBuilder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderAppendBool)
{
  struct R_CSTL_StringBuilder* pBuilder = R_CSTL_NewStringBuilder ();
  ASSERT_NE (nullptr, pBuilder);

  ASSERT_EQ (0, R_CSTL_StringBuilderAppendBool (pBuilder, true));
  ASSERT_EQ (0, R_CSTL_StringBuilderAppendBool (pBuilder, false));

  struct R_CSTL_String* pString = R_CSTL_StringBuilderToString (pBuilder);
  ASSERT_NE (nullptr, pString);
  ExpectStringData (pString, "truefalse");

  R_CSTL_StringDelete (pString);
  R_CSTL_DeleteStringBuilder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderEmplace)
{
  struct R_CSTL_StringBuilder* pBuilder = R_CSTL_NewStringBuilder ();
  ASSERT_NE (nullptr, pBuilder);

  ASSERT_EQ (0, R_CSTL_StringBuilderEmplace (pBuilder, "Hello"));
  ASSERT_EQ (0, R_CSTL_StringBuilderEmplaceSized (pBuilder, "World", 5));

  struct R_CSTL_String* pString = R_CSTL_StringBuilderToString (pBuilder);
  ASSERT_NE (nullptr, pString);
  ExpectStringData (pString, "HelloWorld");

  R_CSTL_StringDelete (pString);
  R_CSTL_DeleteStringBuilder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderInsert)
{
  struct R_CSTL_StringBuilder* pBuilder = R_CSTL_NewStringBuilder ();
  ASSERT_NE (nullptr, pBuilder);

  ASSERT_EQ (0, R_CSTL_StringBuilderAppendData (pBuilder, "World", 5));
  ASSERT_EQ (0, R_CSTL_StringBuilderEmplaceInsert (pBuilder, 0, "Hello"));

  struct R_CSTL_String* pString = R_CSTL_StringBuilderToString (pBuilder);
  ASSERT_NE (nullptr, pString);
  ExpectStringData (pString, "HelloWorld");

  R_CSTL_StringDelete (pString);
  R_CSTL_DeleteStringBuilder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderDelete)
{
  struct R_CSTL_StringBuilder* pBuilder = R_CSTL_NewStringBuilder ();
  ASSERT_NE (nullptr, pBuilder);

  ASSERT_EQ (0, R_CSTL_StringBuilderAppendData (pBuilder, "HelloWorld", 10));
  ASSERT_EQ (0, R_CSTL_StringBuilderDelete (pBuilder, 5, 10));

  struct R_CSTL_String* pString = R_CSTL_StringBuilderToString (pBuilder);
  ASSERT_NE (nullptr, pString);
  ExpectStringData (pString, "Hello");

  R_CSTL_StringDelete (pString);
  R_CSTL_DeleteStringBuilder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderReplace)
{
  struct R_CSTL_StringBuilder* pBuilder = R_CSTL_NewStringBuilder ();
  ASSERT_NE (nullptr, pBuilder);

  ASSERT_EQ (0, R_CSTL_StringBuilderAppendData (pBuilder, "HelloWorld", 10));
  ASSERT_EQ (0, R_CSTL_StringBuilderEmplaceReplace (pBuilder, 5, 10, "Universe"));

  struct R_CSTL_String* pString = R_CSTL_StringBuilderToString (pBuilder);
  ASSERT_NE (nullptr, pString);
  ExpectStringData (pString, "HelloUniverse");

  R_CSTL_StringDelete (pString);
  R_CSTL_DeleteStringBuilder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderReverse)
{
  struct R_CSTL_StringBuilder* pBuilder = R_CSTL_NewStringBuilder ();
  ASSERT_NE (nullptr, pBuilder);

  ASSERT_EQ (0, R_CSTL_StringBuilderAppendData (pBuilder, "Hello", 5));
  ASSERT_EQ (0, R_CSTL_StringBuilderReverse (pBuilder));

  struct R_CSTL_String* pString = R_CSTL_StringBuilderToString (pBuilder);
  ASSERT_NE (nullptr, pString);
  ExpectStringData (pString, "olleH");

  R_CSTL_StringDelete (pString);
  R_CSTL_DeleteStringBuilder (pBuilder);
}

TEST_F (CstlStringTest, GettersReturnZeroOrNullForNullString)
{
  EXPECT_EQ (nullptr, R_CSTL_StringData (nullptr));
  EXPECT_EQ (0u, R_CSTL_StringLength (nullptr));
  EXPECT_EQ (0x00, R_CSTL_StringCharAt (nullptr, 0));
}
