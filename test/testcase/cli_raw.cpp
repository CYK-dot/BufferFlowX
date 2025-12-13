/**
 * @file CliRawDT.cpp
 * @author CYK-Dot
 * @brief Brief description
 * @version 0.1
 * @date 2025-12-08
 *
 * @copyright Copyright (c) 2025 CYK-Dot, MIT License.
 */

/* Header import ------------------------------------------------------------------*/
#include <gtest/gtest.h>
#include "bfx_cli_raw.h"

/* Config macros ------------------------------------------------------------------*/

/* Mock variables and functions  --------------------------------------------------*/

/* Test suites --------------------------------------------------------------------*/

/* Test cases ---------------------------------------------------------------------*/

TEST(CliRawDT, MatchKey) {
    char toMatch[] = "hello world\n";
    char fmt[] = "hello world";
    uint16_t paramIndex[15] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 15);
    EXPECT_EQ(retVal, 0);
}

TEST(CliRawDT, MatchKeyParamDescEnd) {
    char fmt[] = "hello world $index";
    char toMatch[] = "hello world 15\n";
    uint16_t paramIndex[15] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 15);
    EXPECT_EQ(retVal, 1);
    EXPECT_EQ(paramIndex[0], 12);
    EXPECT_EQ(toMatch[paramIndex[0] + 2], '\0');
}

TEST(CliRawDT, MatchKeyParamRawEnd) {
    char fmt[] = "hello world $";
    char toMatch[] = "hello world 15\n";
    uint16_t paramIndex[15] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 15);
    EXPECT_EQ(retVal, 1);
    EXPECT_EQ(paramIndex[0], 12);
    EXPECT_EQ(toMatch[paramIndex[0] + 2], '\0');
}

TEST(CliRawDT, MatchKeyParamDescCenter) {
    char fmt[] = "hello $index world";
    char toMatch[] = "hello 15 world\n";
    uint16_t paramIndex[15] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 15);
    EXPECT_EQ(retVal, 1);
    EXPECT_EQ(paramIndex[0], 6);
    EXPECT_EQ(toMatch[paramIndex[0] + 2], '\0');
}

TEST(CliRawDT, MatchKeyParamRawCenter) {
    char fmt[] = "hello $ world";
    char toMatch[] = "hello 15 world\n";
    uint16_t paramIndex[15] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 15);
    EXPECT_EQ(retVal, 1);
    EXPECT_EQ(paramIndex[0], 6);
    EXPECT_EQ(toMatch[paramIndex[0] + 2], '\0');
}

TEST(CliRawDT, MatchKeyParamDescStart) {
    char fmt[] = "$index world";
    char toMatch[] = "15 world\n";
    uint16_t paramIndex[15] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 15);
    EXPECT_EQ(retVal, 1);
    EXPECT_EQ(paramIndex[0], 0);
    EXPECT_EQ(toMatch[paramIndex[0] + 2], '\0');
}

TEST(CliRawDT, MatchKeyParamRawStart) {
    char fmt[] = "$ world";
    char toMatch[] = "15 world\n";
    uint16_t paramIndex[15] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 15);
    EXPECT_EQ(retVal, 1);
    EXPECT_EQ(paramIndex[0], 0);
    EXPECT_EQ(toMatch[paramIndex[0] + 2], '\0');
}

TEST(CliRawDT, MatchKeyParamDescMulti) {
    char fmt[] = "$index $index world";
    char toMatch[] = "15 hello world\n";
    uint16_t paramIndex[4] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 4);
    EXPECT_EQ(retVal, 2);
    EXPECT_EQ(paramIndex[0], 0);
    EXPECT_EQ(paramIndex[1], 3);
    EXPECT_EQ(toMatch[paramIndex[0] + 2], '\0');
    EXPECT_EQ(toMatch[paramIndex[1] + 5], '\0');
}

TEST(CliRawDT, MaxParamsExact) {
    char fmt[] = "$ $ $ $ $";
    char toMatch[] = "1 2 3 4 5\n";
    uint16_t paramIndex[5] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 5);
    EXPECT_EQ(retVal, 5);
    EXPECT_EQ(paramIndex[0], 0);
    EXPECT_EQ(paramIndex[1], 2);
    EXPECT_EQ(paramIndex[2], 4);
    EXPECT_EQ(paramIndex[3], 6);
    EXPECT_EQ(paramIndex[4], 8);
    EXPECT_EQ(toMatch[paramIndex[0] + 1], '\0');
    EXPECT_EQ(toMatch[paramIndex[1] + 1], '\0');
    EXPECT_EQ(toMatch[paramIndex[2] + 1], '\0');
    EXPECT_EQ(toMatch[paramIndex[3] + 1], '\0');
    EXPECT_EQ(toMatch[paramIndex[4] + 1], '\0');
}

TEST(CliRawDT, OverFlowAvoid) {
    char fmt[] = "$index $index $index $index world";
    char toMatch[] = "15 hello world yes\n";
    uint16_t paramIndex[4] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 4);
    EXPECT_EQ(retVal, UINT16_MAX);
}

TEST(CliRawDT, EmptyStrings) {
    char toMatch[] = "\n";
    char fmt[] = "";
    uint16_t paramIndex[15] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 15);
    EXPECT_EQ(retVal, 0);
}

TEST(CliRawDT, EmptyToMatchWithParam) {
    char toMatch[] = "\n";
    char fmt[] = "$index";
    uint16_t paramIndex[15] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 15);
    EXPECT_EQ(retVal, UINT16_MAX);
}

TEST(CliRawDT, LeadingTrailingSpaces) {
    char toMatch[] = " hello world \n";
    char fmt[] = "hello world";
    uint16_t paramIndex[15] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 15);
    EXPECT_EQ(retVal, 0);
}

TEST(CliRaw, UnicodeChars) {
    char fmt[] = "hello $";
    char toMatch[] = "hello 中文\n";
    uint16_t paramIndex[15] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 15);
    EXPECT_EQ(retVal, 1);
    EXPECT_EQ(paramIndex[0], 6);
}

TEST(CliRaw, PartialMatchFailure) {
    char fmt[] = "hello world extra";
    char toMatch[] = "hello world\n";
    uint16_t paramIndex[15] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 15);
    EXPECT_EQ(retVal, UINT16_MAX);
}

TEST(CliRaw, ParamAtEndNoInput) {
    char fmt[] = "cmd $";
    char toMatch[] = "cmd\n";
    uint16_t paramIndex[15] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 15);
    EXPECT_EQ(retVal, UINT16_MAX);
}

TEST(CliRaw, SpaceMoreThanOne) {
    char fmt[] = "hello world";
    char toMatch[] = "hello   world\n";
    uint16_t paramIndex[15] = {0};
    uint16_t retVal = BFX_CliRawMatch(toMatch, fmt, paramIndex, 15);
    EXPECT_EQ(retVal, 0);
}
