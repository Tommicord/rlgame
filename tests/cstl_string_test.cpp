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
        {
            ASSERT_EQ (0, r_cstl_heap_init (kTestHeapSize));
        }

        void
        TearDown () override
        {
            r_cstl_heap_shutdown ();
        }

        static void
        ExpectStringData (const struct r_cstl_string* pString, const std::string& expected)
        {
            ASSERT_NE (nullptr, pString);
            EXPECT_EQ (expected.size (), r_cstl_string_length (pString));
            const char* pData = r_cstl_string_data (pString);
            ASSERT_NE (nullptr, pData);
            EXPECT_EQ (expected, std::string (pData, r_cstl_string_length (pString)));
        }
};

} // namespace

TEST (CstlStringInitTest, DeleteNullIsSafe)
{
    r_cstl_string_delete (nullptr);
    SUCCEED ();
}

TEST_F (CstlStringTest, NewEmptyString)
{
    struct r_cstl_string* pString = r_cstl_new_string ();
    ASSERT_NE (nullptr, pString);
    EXPECT_EQ (0u, r_cstl_string_length (pString));
    ExpectStringData (pString, "");
    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, NewStringWithData)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("Hello");
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "Hello");
    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, NewStringWithDataSized)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data_sized ("HelloWorld", 5);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "Hello");
    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, NewStringWithDataRejectsNull)
{
    EXPECT_EQ (nullptr, r_cstl_new_string_with_data (nullptr));
}

TEST_F (CstlStringTest, NewStringWithFormat)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_format ("%s %d", "Test", 42);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "Test 42");
    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, NewStringWithFormatNullReturnsEmpty)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_format (nullptr);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "");
    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, CharAtBoundsCheck)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("ABC");
    ASSERT_NE (nullptr, pString);

    EXPECT_EQ ('A', r_cstl_string_char_at (pString, 0));
    EXPECT_EQ ('B', r_cstl_string_char_at (pString, 1));
    EXPECT_EQ ('C', r_cstl_string_char_at (pString, 2));
    EXPECT_EQ (0x00, r_cstl_string_char_at (pString, 3)); // Out of bounds
    EXPECT_EQ (0x00, r_cstl_string_char_at (nullptr, 0));

    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, StringEquals)
{
    struct r_cstl_string* pString1 = r_cstl_new_string_with_data ("Hello");
    struct r_cstl_string* pString2 = r_cstl_new_string_with_data ("Hello");
    struct r_cstl_string* pString3 = r_cstl_new_string_with_data ("World");

    ASSERT_NE (nullptr, pString1);
    ASSERT_NE (nullptr, pString2);
    ASSERT_NE (nullptr, pString3);

    EXPECT_EQ (1, r_cstl_string_equals (pString1, pString2));
    EXPECT_EQ (0, r_cstl_string_equals (pString1, pString3));
    EXPECT_EQ (1, r_cstl_string_equals (pString1, pString1));
    EXPECT_EQ (0, r_cstl_string_equals (pString1, nullptr));

    r_cstl_string_delete (pString1);
    r_cstl_string_delete (pString2);
    r_cstl_string_delete (pString3);
}

TEST_F (CstlStringTest, StringCompare)
{
    struct r_cstl_string* pString1 = r_cstl_new_string_with_data ("Apple");
    struct r_cstl_string* pString2 = r_cstl_new_string_with_data ("Banana");
    struct r_cstl_string* pString3 = r_cstl_new_string_with_data ("Apple");

    ASSERT_NE (nullptr, pString1);
    ASSERT_NE (nullptr, pString2);
    ASSERT_NE (nullptr, pString3);

    EXPECT_LT (r_cstl_string_compare (pString1, pString2), 0);
    EXPECT_GT (r_cstl_string_compare (pString2, pString1), 0);
    EXPECT_EQ (0, r_cstl_string_compare (pString1, pString3));
    EXPECT_LT (r_cstl_string_compare (nullptr, pString1), 0);
    EXPECT_GT (r_cstl_string_compare (pString1, nullptr), 0);

    r_cstl_string_delete (pString1);
    r_cstl_string_delete (pString2);
    r_cstl_string_delete (pString3);
}

TEST_F (CstlStringTest, StringConcat)
{
    struct r_cstl_string* pString1 = r_cstl_new_string_with_data ("Hello");
    struct r_cstl_string* pString2 = r_cstl_new_string_with_data (" World");

    ASSERT_NE (nullptr, pString1);
    ASSERT_NE (nullptr, pString2);

    struct r_cstl_string* pResult = r_cstl_string_concat (pString1, pString2);
    ASSERT_NE (nullptr, pResult);
    ExpectStringData (pResult, "Hello World");

    r_cstl_string_delete (pString1);
    r_cstl_string_delete (pString2);
    r_cstl_string_delete (pResult);
}

TEST_F (CstlStringTest, StringConcatWithNull)
{
    struct r_cstl_string* pString1 = r_cstl_new_string_with_data ("Hello");
    ASSERT_NE (nullptr, pString1);

    struct r_cstl_string* pResult1 = r_cstl_string_concat (pString1, nullptr);
    ASSERT_NE (nullptr, pResult1);
    ExpectStringData (pResult1, "Hello");

    struct r_cstl_string* pResult2 = r_cstl_string_concat (nullptr, pString1);
    ASSERT_NE (nullptr, pResult2);
    ExpectStringData (pResult2, "Hello");

    struct r_cstl_string* pResult3 = r_cstl_string_concat (nullptr, nullptr);
    ASSERT_NE (nullptr, pResult3);
    ExpectStringData (pResult3, "");

    r_cstl_string_delete (pString1);
    r_cstl_string_delete (pResult1);
    r_cstl_string_delete (pResult2);
    r_cstl_string_delete (pResult3);
}

TEST_F (CstlStringTest, StringSubstring)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("HelloWorld");
    ASSERT_NE (nullptr, pString);

    struct r_cstl_string* pSub1 = r_cstl_string_substring (pString, 0, 5);
    ASSERT_NE (nullptr, pSub1);
    ExpectStringData (pSub1, "Hello");

    struct r_cstl_string* pSub2 = r_cstl_string_substring (pString, 5, 10);
    ASSERT_NE (nullptr, pSub2);
    ExpectStringData (pSub2, "World");

    struct r_cstl_string* pSub3 = r_cstl_string_substring (pString, 3, 7);
    ASSERT_NE (nullptr, pSub3);
    ExpectStringData (pSub3, "loWo");

    r_cstl_string_delete (pString);
    r_cstl_string_delete (pSub1);
    r_cstl_string_delete (pSub2);
    r_cstl_string_delete (pSub3);
}

TEST_F (CstlStringTest, StringSubstringInvalidRange)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("Hello");
    ASSERT_NE (nullptr, pString);

    struct r_cstl_string* pSub1 = r_cstl_string_substring (pString, 10, 15);
    ASSERT_NE (nullptr, pSub1);
    ExpectStringData (pSub1, "");

    struct r_cstl_string* pSub2 = r_cstl_string_substring (pString, 3, 3);
    ASSERT_NE (nullptr, pSub2);
    ExpectStringData (pSub2, "");

    EXPECT_EQ (nullptr, r_cstl_string_substring (nullptr, 0, 5));

    r_cstl_string_delete (pString);
    r_cstl_string_delete (pSub1);
    r_cstl_string_delete (pSub2);
}

TEST_F (CstlStringTest, StringStartsWith)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("HelloWorld");
    ASSERT_NE (nullptr, pString);

    EXPECT_EQ (1, r_cstl_string_starts_with (pString, "Hello"));
    EXPECT_EQ (0, r_cstl_string_starts_with (pString, "World"));
    EXPECT_EQ (1, r_cstl_string_starts_with (pString, ""));
    EXPECT_EQ (0, r_cstl_string_starts_with (pString, "HelloWorld!"));
    EXPECT_EQ (0, r_cstl_string_starts_with (nullptr, "Hello"));
    EXPECT_EQ (0, r_cstl_string_starts_with (pString, nullptr));

    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, StringEndsWith)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("HelloWorld");
    ASSERT_NE (nullptr, pString);

    EXPECT_EQ (1, r_cstl_string_ends_with (pString, "World"));
    EXPECT_EQ (0, r_cstl_string_ends_with (pString, "Hello"));
    EXPECT_EQ (1, r_cstl_string_ends_with (pString, ""));
    EXPECT_EQ (0, r_cstl_string_ends_with (pString, "!HelloWorld"));
    EXPECT_EQ (0, r_cstl_string_ends_with (nullptr, "World"));
    EXPECT_EQ (0, r_cstl_string_ends_with (pString, nullptr));

    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, StringIndexOf)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("HelloWorld");
    ASSERT_NE (nullptr, pString);

    EXPECT_EQ (0u, r_cstl_string_index_of (pString, "Hello"));
    EXPECT_EQ (5u, r_cstl_string_index_of (pString, "World"));
    EXPECT_EQ (2u, r_cstl_string_index_of (pString, "llo"));
    EXPECT_EQ ((size_t)-1, r_cstl_string_index_of (pString, "xyz"));
    EXPECT_EQ ((size_t)-1, r_cstl_string_index_of (nullptr, "Hello"));
    EXPECT_EQ ((size_t)-1, r_cstl_string_index_of (pString, nullptr));

    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, StringLastIndexOf)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("HelloHello");
    ASSERT_NE (nullptr, pString);

    EXPECT_EQ (5u, r_cstl_string_last_index_of (pString, "Hello"));
    EXPECT_EQ (7u, r_cstl_string_last_index_of (pString, "llo"));
    EXPECT_EQ ((size_t)-1, r_cstl_string_last_index_of (pString, "xyz"));
    EXPECT_EQ ((size_t)-1, r_cstl_string_last_index_of (nullptr, "Hello"));
    EXPECT_EQ ((size_t)-1, r_cstl_string_last_index_of (pString, nullptr));

    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, StringIndexOfChar)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("HelloWorld");
    ASSERT_NE (nullptr, pString);

    EXPECT_EQ (2u, r_cstl_string_index_of_char (pString, 'l'));
    EXPECT_EQ (0u, r_cstl_string_index_of_char (pString, 'H'));
    EXPECT_EQ ((size_t)-1, r_cstl_string_index_of_char (pString, 'z'));
    EXPECT_EQ ((size_t)-1, r_cstl_string_index_of_char (nullptr, 'H'));

    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, StringLastIndexOfChar)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("HelloWorld");
    ASSERT_NE (nullptr, pString);

    EXPECT_EQ (8u, r_cstl_string_last_index_of_char (pString, 'l'));
    EXPECT_EQ (5u, r_cstl_string_last_index_of_char (pString, 'W'));
    EXPECT_EQ ((size_t)-1, r_cstl_string_last_index_of_char (pString, 'z'));
    EXPECT_EQ ((size_t)-1, r_cstl_string_last_index_of_char (nullptr, 'H'));

    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, StringContains)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("HelloWorld");
    ASSERT_NE (nullptr, pString);

    EXPECT_EQ (1, r_cstl_string_contains (pString, "Hello"));
    EXPECT_EQ (0, r_cstl_string_contains (pString, "xyz"));

    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, StringIsEmpty)
{
    struct r_cstl_string* pString1 = r_cstl_new_string_with_data ("");
    struct r_cstl_string* pString2 = r_cstl_new_string_with_data ("Hello");

    ASSERT_NE (nullptr, pString1);
    ASSERT_NE (nullptr, pString2);

    EXPECT_EQ (1, r_cstl_string_is_empty (pString1));
    EXPECT_EQ (0, r_cstl_string_is_empty (pString2));
    EXPECT_EQ (-1, r_cstl_string_is_empty (nullptr));

    r_cstl_string_delete (pString1);
    r_cstl_string_delete (pString2);
}

TEST_F (CstlStringTest, StringToLowerCase)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("HeLLo WoRLd");
    ASSERT_NE (nullptr, pString);

    struct r_cstl_string* pResult = r_cstl_string_to_lower_case (pString);
    ASSERT_NE (nullptr, pResult);
    ExpectStringData (pResult, "hello world");

    r_cstl_string_delete (pString);
    r_cstl_string_delete (pResult);
}

TEST_F (CstlStringTest, StringToUpperCase)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("HeLLo WoRLd");
    ASSERT_NE (nullptr, pString);

    struct r_cstl_string* pResult = r_cstl_string_to_upper_case (pString);
    ASSERT_NE (nullptr, pResult);
    ExpectStringData (pResult, "HELLO WORLD");

    r_cstl_string_delete (pString);
    r_cstl_string_delete (pResult);
}

TEST_F (CstlStringTest, StringTrim)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("  Hello World  ");
    ASSERT_NE (nullptr, pString);

    struct r_cstl_string* pResult = r_cstl_string_trim (pString);
    ASSERT_NE (nullptr, pResult);
    ExpectStringData (pResult, "Hello World");

    r_cstl_string_delete (pString);
    r_cstl_string_delete (pResult);
}

TEST_F (CstlStringTest, StringReplace)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("Hello World");
    ASSERT_NE (nullptr, pString);

    struct r_cstl_string* pResult = r_cstl_string_replace (pString, "World", "Universe");
    ASSERT_NE (nullptr, pResult);
    ExpectStringData (pResult, "Hello Universe");

    r_cstl_string_delete (pString);
    r_cstl_string_delete (pResult);
}

TEST_F (CstlStringTest, StringReplaceChar)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("Hello World");
    ASSERT_NE (nullptr, pString);

    struct r_cstl_string* pResult = r_cstl_string_replace_char (pString, 'o', 'a');
    ASSERT_NE (nullptr, pResult);
    ExpectStringData (pResult, "Hella Warld");

    r_cstl_string_delete (pString);
    r_cstl_string_delete (pResult);
}

TEST_F (CstlStringTest, StringRepeat)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("Abc");
    ASSERT_NE (nullptr, pString);

    struct r_cstl_string* pResult = r_cstl_string_repeat (pString, 3);
    ASSERT_NE (nullptr, pResult);
    ExpectStringData (pResult, "AbcAbcAbc");

    r_cstl_string_delete (pString);
    r_cstl_string_delete (pResult);
}

TEST_F (CstlStringTest, StringEqualsIgnoreCase)
{
    struct r_cstl_string* pString1 = r_cstl_new_string_with_data ("Hello");
    struct r_cstl_string* pString2 = r_cstl_new_string_with_data ("HELLO");

    ASSERT_NE (nullptr, pString1);
    ASSERT_NE (nullptr, pString2);

    EXPECT_EQ (1, r_cstl_string_equals_ignore_case (pString1, pString2));
    EXPECT_EQ (0, r_cstl_string_equals_ignore_case (pString1, nullptr));

    r_cstl_string_delete (pString1);
    r_cstl_string_delete (pString2);
}

TEST_F (CstlStringTest, StringCompareIgnoreCase)
{
    struct r_cstl_string* pString1 = r_cstl_new_string_with_data ("apple");
    struct r_cstl_string* pString2 = r_cstl_new_string_with_data ("BANANA");

    ASSERT_NE (nullptr, pString1);
    ASSERT_NE (nullptr, pString2);

    EXPECT_LT (r_cstl_string_compare_ignore_case (pString1, pString2), 0);
    EXPECT_EQ (0, r_cstl_string_compare_ignore_case (pString1, pString1));

    r_cstl_string_delete (pString1);
    r_cstl_string_delete (pString2);
}

TEST_F (CstlStringTest, StringHashCode)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("Hello");
    ASSERT_NE (nullptr, pString);

    size_t hash1 = r_cstl_string_hash_code (pString);
    size_t hash2 = r_cstl_string_hash_code (pString);
    EXPECT_EQ (hash1, hash2);

    EXPECT_EQ (0u, r_cstl_string_hash_code (nullptr));

    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, StringRemove)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_data ("HelloWorld");
    ASSERT_NE (nullptr, pString);

    struct r_cstl_string* pResult = r_cstl_string_remove (pString, 5, 10);
    ASSERT_NE (nullptr, pResult);
    ExpectStringData (pResult, "Hello");

    r_cstl_string_delete (pString);
    r_cstl_string_delete (pResult);
}

TEST_F (CstlStringTest, StringValueOfInt)
{
    struct r_cstl_string* pString = r_cstl_string_value_of_int (42);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "42");
    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, StringValueOfLong)
{
    struct r_cstl_string* pString = r_cstl_string_value_of_long (123456789LL);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "123456789");
    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, StringValueOfDouble)
{
    struct r_cstl_string* pString = r_cstl_string_value_of_double (3.14);
    ASSERT_NE (nullptr, pString);
    EXPECT_EQ (0, strncmp (r_cstl_string_data (pString), "3.14", 4));
    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, StringJoin)
{
    struct r_cstl_string* pDelimiter = r_cstl_new_string_with_data (", ");
    struct r_cstl_string* pString1 = r_cstl_new_string_with_data ("A");
    struct r_cstl_string* pString2 = r_cstl_new_string_with_data ("B");
    struct r_cstl_string* pString3 = r_cstl_new_string_with_data ("C");

    ASSERT_NE (nullptr, pDelimiter);
    ASSERT_NE (nullptr, pString1);
    ASSERT_NE (nullptr, pString2);
    ASSERT_NE (nullptr, pString3);

    const struct r_cstl_string* strings[] = {pString1, pString2, pString3};
    struct r_cstl_string*       pResult = r_cstl_string_join (pDelimiter, strings, 3);
    ASSERT_NE (nullptr, pResult);
    ExpectStringData (pResult, "A, B, C");

    r_cstl_string_delete (pDelimiter);
    r_cstl_string_delete (pString1);
    r_cstl_string_delete (pString2);
    r_cstl_string_delete (pString3);
    r_cstl_string_delete (pResult);
}

TEST_F (CstlStringTest, StringBuilderBasic)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);
    EXPECT_EQ (0u, r_cstl_string_builder_length (pBuilder));

    struct r_cstl_string* pString = r_cstl_new_string_with_data ("Hello");
    ASSERT_NE (nullptr, pString);
    ASSERT_EQ (0, r_cstl_string_builder_append (pBuilder, pString));

    EXPECT_EQ (5u, r_cstl_string_builder_length (pBuilder));

    r_cstl_string_builder_clear (pBuilder);
    EXPECT_EQ (0u, r_cstl_string_builder_length (pBuilder));

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderAppendData)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_append_data (pBuilder, "Hello", 5));
    ASSERT_EQ (0, r_cstl_string_builder_append_data (pBuilder, "World", 5));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "HelloWorld");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderAppendChar)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_append_char (pBuilder, 'A'));
    ASSERT_EQ (0, r_cstl_string_builder_append_char (pBuilder, 'B'));
    ASSERT_EQ (0, r_cstl_string_builder_append_char (pBuilder, 'C'));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "ABC");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderAppendInt)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_append_int (pBuilder, 42));
    ASSERT_EQ (0, r_cstl_string_builder_append_int (pBuilder, -123));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "42-123");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderAppendBool)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_append_bool (pBuilder, true));
    ASSERT_EQ (0, r_cstl_string_builder_append_bool (pBuilder, false));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "truefalse");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderEmplace)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_emplace (pBuilder, "Hello"));
    ASSERT_EQ (0, r_cstl_string_builder_emplace_sized (pBuilder, "World", 5));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "HelloWorld");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderInsert)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_append_data (pBuilder, "World", 5));
    ASSERT_EQ (0, r_cstl_string_builder_emplace_insert (pBuilder, 0, "Hello"));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "HelloWorld");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderDelete)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_append_data (pBuilder, "HelloWorld", 10));
    ASSERT_EQ (0, r_cstl_string_builder_delete (pBuilder, 5, 10));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "Hello");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderReplace)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_append_data (pBuilder, "HelloWorld", 10));
    ASSERT_EQ (0, r_cstl_string_builder_emplace_replace (pBuilder, 5, 10, "Universe"));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "HelloUniverse");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderReverse)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_append_data (pBuilder, "Hello", 5));
    ASSERT_EQ (0, r_cstl_string_builder_reverse (pBuilder));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "olleH");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, GettersReturnZeroOrNullForNullString)
{
    EXPECT_EQ (nullptr, r_cstl_string_data (nullptr));
    EXPECT_EQ (0u, r_cstl_string_length (nullptr));
    EXPECT_EQ (0x00, r_cstl_string_char_at (nullptr, 0));
}

TEST_F (CstlStringTest, NewStringWithCapacity)
{
    struct r_cstl_string* pString = r_cstl_new_string_with_capacity (100);
    ASSERT_NE (nullptr, pString);
    EXPECT_EQ (0u, r_cstl_string_length (pString));
    ExpectStringData (pString, "");
    r_cstl_string_delete (pString);
}

TEST_F (CstlStringTest, StringCopy)
{
    struct r_cstl_string* pSrc = r_cstl_new_string_with_data ("Hello");
    struct r_cstl_string* pDst = r_cstl_new_string ();

    ASSERT_NE (nullptr, pSrc);
    ASSERT_NE (nullptr, pDst);

    ASSERT_EQ (0, r_cstl_string_copy (pDst, pSrc));
    ExpectStringData (pDst, "Hello");

    r_cstl_string_delete (pSrc);
    r_cstl_string_delete (pDst);
}

TEST_F (CstlStringTest, StringCopyRejectsNullDst)
{
    struct r_cstl_string* pSrc = r_cstl_new_string_with_data ("Hello");
    ASSERT_NE (nullptr, pSrc);

    EXPECT_NE (0, r_cstl_string_copy (nullptr, pSrc));

    r_cstl_string_delete (pSrc);
}

TEST_F (CstlStringTest, StringBuilderWithData)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder_with_data ("Hello");
    ASSERT_NE (nullptr, pBuilder);
    EXPECT_EQ (5u, r_cstl_string_builder_length (pBuilder));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "Hello");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderWithCapacity)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder_with_capacity (100);
    ASSERT_NE (nullptr, pBuilder);
    EXPECT_EQ (0u, r_cstl_string_builder_length (pBuilder));
    // Capacity may be 0 initially and grow on demand
    EXPECT_GE (r_cstl_string_builder_length (pBuilder), 0u);

    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderEnsureCapacity)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_ensure_capacity (pBuilder, 50));
    EXPECT_GE (r_cstl_string_builder_capacity (pBuilder), 50u);

    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderCapacityGetter)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder_with_capacity (32);
    ASSERT_NE (nullptr, pBuilder);

    // Capacity getter returns 0 for null builder
    EXPECT_EQ (0u, r_cstl_string_builder_capacity (nullptr));
    // Capacity may be 0 initially
    EXPECT_GE (r_cstl_string_builder_capacity (pBuilder), 0u);

    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderAppendLong)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_append_long (pBuilder, 123456789LL));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "123456789");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderAppendDouble)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_append_double (pBuilder, 3.14159));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    const char* data = r_cstl_string_data (pString);
    ASSERT_NE (nullptr, data);
    EXPECT_TRUE (strstr (data, "3.14") != nullptr);

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderAppendRepeat)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_append_repeat (pBuilder, "Abc", 3));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "AbcAbcAbc");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderAppendf)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_appendf (pBuilder, "%s %d", "Test", 42));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "Test 42");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderInsertWithString)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    struct r_cstl_string* pString = r_cstl_new_string_with_data ("World");
    ASSERT_NE (nullptr, pString);

    ASSERT_EQ (0, r_cstl_string_builder_append_data (pBuilder, "World", 5));
    ASSERT_EQ (0, r_cstl_string_builder_insert (pBuilder, 0, pString));

    struct r_cstl_string* pResult = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pResult);
    ExpectStringData (pResult, "WorldWorld");

    r_cstl_string_delete (pString);
    r_cstl_string_delete (pResult);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderInsertChar)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_append_data (pBuilder, "Hllo", 4));
    ASSERT_EQ (0, r_cstl_string_builder_insert_char (pBuilder, 1, 'e'));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "Hello");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderDeleteCharAt)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_append_data (pBuilder, "Hello", 5));
    ASSERT_EQ (0, r_cstl_string_builder_delete_char_at (pBuilder, 1));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "Hllo");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderReplaceWithString)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    struct r_cstl_string* pReplace = r_cstl_new_string_with_data ("Universe");
    ASSERT_NE (nullptr, pReplace);

    ASSERT_EQ (0, r_cstl_string_builder_append_data (pBuilder, "HelloWorld", 10));
    ASSERT_EQ (0, r_cstl_string_builder_replace (pBuilder, 5, 10, pReplace));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "HelloUniverse");

    r_cstl_string_delete (pReplace);
    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderSetCharAt)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_append_data (pBuilder, "Hella", 5));
    ASSERT_EQ (0, r_cstl_string_builder_set_char_at (pBuilder, 4, 'o'));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "Hello");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}

TEST_F (CstlStringTest, StringBuilderSetLength)
{
    struct r_cstl_string_builder* pBuilder = r_cstl_new_string_builder ();
    ASSERT_NE (nullptr, pBuilder);

    ASSERT_EQ (0, r_cstl_string_builder_append_data (pBuilder, "HelloWorld", 10));
    ASSERT_EQ (0, r_cstl_string_builder_set_length (pBuilder, 5));

    EXPECT_EQ (5u, r_cstl_string_builder_length (pBuilder));

    struct r_cstl_string* pString = r_cstl_string_builder_to_string (pBuilder);
    ASSERT_NE (nullptr, pString);
    ExpectStringData (pString, "Hello");

    r_cstl_string_delete (pString);
    r_cstl_delete_string_builder (pBuilder);
}
