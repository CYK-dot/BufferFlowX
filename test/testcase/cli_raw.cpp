/**
 * @file CliRaw.cpp
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

TEST(CliRaw, MatchKey) {
    char toMatch[] = "hello world";
    char fmt[] = "hello world";
    uint16_t paramIndex = 0;
    uint16_t retVal = BFX_CliMatch(toMatch, fmt, &paramIndex, 1);
    EXPECT_EQ(retVal, 0);
}

TEST(CliRaw, MatchKeyParamDescEnd) {
    char fmt[] = "hello world $index";
    char toMatch[] = "hello world 15\n";
    uint16_t paramIndex = 0;
    uint16_t retVal = BFX_CliMatch(toMatch, fmt, &paramIndex, 1);
    EXPECT_EQ(retVal, 1);
    EXPECT_EQ(paramIndex, 12);
}

TEST(CliRaw, MatchKeyParamRawEnd) {
    char fmt[] = "hello world $";
    char toMatch[] = "hello world 15\n";
    uint16_t paramIndex = 0;
    uint16_t retVal = BFX_CliMatch(toMatch, fmt, &paramIndex, 1);
    EXPECT_EQ(retVal, 1);
    EXPECT_EQ(paramIndex, 12);
}

TEST(CliRaw, MatchKeyParamDescCenter) {
    char fmt[] = "hello $index world";
    char toMatch[] = "hello 15 world\n";
    uint16_t paramIndex = 0;
    uint16_t retVal = BFX_CliMatch(toMatch, fmt, &paramIndex, 1);
    EXPECT_EQ(retVal, 1);
    EXPECT_EQ(paramIndex, 6);
}

TEST(CliRaw, MatchKeyParamRawCenter) {
    char fmt[] = "hello $ world";
    char toMatch[] = "hello 15 world\n";
    uint16_t paramIndex = 0;
    uint16_t retVal = BFX_CliMatch(toMatch, fmt, &paramIndex, 1);
    EXPECT_EQ(retVal, 1);
    EXPECT_EQ(paramIndex, 6);
}

TEST(CliRaw, MatchKeyParamDescStart) {
    char fmt[] = "$index world";
    char toMatch[] = "15 world\n";
    uint16_t paramIndex = 0;
    uint16_t retVal = BFX_CliMatch(toMatch, fmt, &paramIndex, 1);
    EXPECT_EQ(retVal, 1);
    EXPECT_EQ(paramIndex, 0);
}

TEST(CliRaw, MatchKeyParamRawStart) {
    char fmt[] = "$ world";
    char toMatch[] = "15 world\n";
    uint16_t paramIndex = 0;
    uint16_t retVal = BFX_CliMatch(toMatch, fmt, &paramIndex, 1);
    EXPECT_EQ(retVal, 1);
    EXPECT_EQ(paramIndex, 0);
}

TEST(CliRaw, MatchKeyParamDescMulti) {
    char fmt[] = "$index $index world";
    char toMatch[] = "15 hello world\n";
    uint16_t paramIndex[2];
    uint16_t retVal = BFX_CliMatch(toMatch, fmt, paramIndex, 2);
    EXPECT_EQ(retVal, 2);
    EXPECT_EQ(paramIndex[0], 0);
    EXPECT_EQ(paramIndex[1], 3);
}
