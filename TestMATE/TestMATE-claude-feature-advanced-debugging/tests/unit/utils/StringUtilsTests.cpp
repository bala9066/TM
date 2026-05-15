/**************************************************************************
 * File Name: StringUtilsTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-XX
 * Description: Unit tests for StringUtils functions
 **************************************************************************/

#include "utils/StringUtils.h"
#include <gtest/gtest.h>

using namespace TestMATE;
using namespace TestMATE::StringUtils;

//=============================================================================
// Trim Tests
//=============================================================================

TEST(StringUtilsTest, Trim_RemovesLeadingAndTrailingWhitespace) {
    EXPECT_EQ(Trim("  hello  "), "hello");
    EXPECT_EQ(Trim("\t\nhello\t\n"), "hello");
    EXPECT_EQ(Trim("hello"), "hello");
    EXPECT_EQ(Trim("   "), "");
    EXPECT_EQ(Trim(""), "");
}

TEST(StringUtilsTest, TrimLeft_RemovesLeadingWhitespace) {
    EXPECT_EQ(TrimLeft("  hello  "), "hello  ");
    EXPECT_EQ(TrimLeft("hello"), "hello");
}

TEST(StringUtilsTest, TrimRight_RemovesTrailingWhitespace) {
    EXPECT_EQ(TrimRight("  hello  "), "  hello");
    EXPECT_EQ(TrimRight("hello"), "hello");
}

//=============================================================================
// Case Conversion Tests
//=============================================================================

TEST(StringUtilsTest, ToLower_ConvertsToLowercase) {
    EXPECT_EQ(ToLower("HELLO"), "hello");
    EXPECT_EQ(ToLower("Hello World"), "hello world");
    EXPECT_EQ(ToLower("hello"), "hello");
    EXPECT_EQ(ToLower("123ABC"), "123abc");
}

TEST(StringUtilsTest, ToUpper_ConvertsToUppercase) {
    EXPECT_EQ(ToUpper("hello"), "HELLO");
    EXPECT_EQ(ToUpper("Hello World"), "HELLO WORLD");
    EXPECT_EQ(ToUpper("HELLO"), "HELLO");
    EXPECT_EQ(ToUpper("123abc"), "123ABC");
}

//=============================================================================
// Split/Join Tests
//=============================================================================

TEST(StringUtilsTest, Split_SplitsByDelimiter) {
    auto result = Split("a,b,c", ',');
    ASSERT_EQ(result.size(), 3u);
    EXPECT_EQ(result[0], "a");
    EXPECT_EQ(result[1], "b");
    EXPECT_EQ(result[2], "c");
}

TEST(StringUtilsTest, Split_EmptyString) {
    auto result = Split("", ',');
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0], "");
}

TEST(StringUtilsTest, Join_JoinsWithDelimiter) {
    TVector<TString> vec = {"a", "b", "c"};
    EXPECT_EQ(Join(vec, ","), "a,b,c");
    EXPECT_EQ(Join(vec, " - "), "a - b - c");
}

TEST(StringUtilsTest, Join_EmptyVector) {
    TVector<TString> vec;
    EXPECT_EQ(Join(vec, ","), "");
}

//=============================================================================
// StartsWith/EndsWith/Contains Tests
//=============================================================================

TEST(StringUtilsTest, StartsWith_ChecksPrefix) {
    EXPECT_TRUE(StartsWith("hello world", "hello"));
    EXPECT_TRUE(StartsWith("hello", "hello"));
    EXPECT_FALSE(StartsWith("hello", "world"));
    EXPECT_FALSE(StartsWith("hi", "hello"));
}

TEST(StringUtilsTest, EndsWith_ChecksSuffix) {
    EXPECT_TRUE(EndsWith("hello world", "world"));
    EXPECT_TRUE(EndsWith("hello", "hello"));
    EXPECT_FALSE(EndsWith("hello", "world"));
    EXPECT_FALSE(EndsWith("hi", "hello"));
}

TEST(StringUtilsTest, Contains_ChecksSubstring) {
    EXPECT_TRUE(Contains("hello world", "lo wo"));
    EXPECT_TRUE(Contains("hello", "hello"));
    EXPECT_FALSE(Contains("hello", "world"));
}

//=============================================================================
// Replace Tests
//=============================================================================

TEST(StringUtilsTest, Replace_ReplacesAllOccurrences) {
    EXPECT_EQ(Replace("hello hello", "hello", "hi"), "hi hi");
    EXPECT_EQ(Replace("aaa", "a", "bb"), "bbbbbb");
    EXPECT_EQ(Replace("hello", "world", "hi"), "hello");
}

//=============================================================================
// IsEmpty Tests
//=============================================================================

TEST(StringUtilsTest, IsEmpty_ChecksEmptyOrWhitespace) {
    EXPECT_TRUE(IsEmpty(""));
    EXPECT_TRUE(IsEmpty("   "));
    EXPECT_TRUE(IsEmpty("\t\n"));
    EXPECT_FALSE(IsEmpty("hello"));
    EXPECT_FALSE(IsEmpty("  a  "));
}

//=============================================================================
// Format Tests
//=============================================================================

TEST(StringUtilsTest, Format_SubstitutesPlaceholders) {
    EXPECT_EQ(Format("Hello, {}!", "World"), "Hello, World!");
    EXPECT_EQ(Format("{} + {} = {}", 1, 2, 3), "1 + 2 = 3");
    EXPECT_EQ(Format("No placeholders"), "No placeholders");
}
