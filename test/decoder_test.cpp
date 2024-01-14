// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <vector>

#include "kodebooth/frame/frame.h"

static constexpr uint8_t NON_SPECIAL_VALUE = 42;
static_assert(NON_SPECIAL_VALUE != kodebooth::frame::STX);
static_assert(NON_SPECIAL_VALUE != kodebooth::frame::ETX);
static_assert(NON_SPECIAL_VALUE != kodebooth::frame::ESC);

class frameDecoderTest
    : public ::testing::TestWithParam<
          std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> {};

TEST_P(frameDecoderTest, DecodeFrame) {
  auto [expected, values] = GetParam();

  kodebooth::frame::Decoder<100> decoder(
      kodebooth::frame::IntegrityCheck::Without);

  for (int i = 0; i < values.size() - 1; i++) {
    auto [buffer, size] = decoder.put(values[i]);
    EXPECT_FALSE(buffer);
    EXPECT_EQ(size, 1);
  }

  auto [buffer, size] = decoder.put(values.back());
  EXPECT_TRUE(buffer);
  EXPECT_EQ(size, expected.size());
  for (int i = 0; i < size; i++) {
    EXPECT_EQ(buffer.value()[i], expected[i]);
  }
}

INSTANTIATE_TEST_SUITE_P(
    frameEncoderTests, frameDecoderTest,
    testing::Values(
        std::make_pair(std::vector<uint8_t>{},
                       std::vector<uint8_t>{kodebooth::frame::STX,
                                            kodebooth::frame::ETX}),
        std::make_pair(std::vector<uint8_t>{NON_SPECIAL_VALUE},
                       std::vector<uint8_t>{kodebooth::frame::STX,
                                            NON_SPECIAL_VALUE,
                                            kodebooth::frame::ETX}),
        std::make_pair(
            std::vector<uint8_t>{kodebooth::frame::STX, kodebooth::frame::ESC,
                                 kodebooth::frame::ETX},
            std::vector<uint8_t>{kodebooth::frame::STX, kodebooth::frame::ESC,
                                 static_cast<uint8_t>(~kodebooth::frame::STX),
                                 kodebooth::frame::ESC,
                                 static_cast<uint8_t>(~kodebooth::frame::ESC),
                                 kodebooth::frame::ESC,
                                 static_cast<uint8_t>(~kodebooth::frame::ETX),
                                 kodebooth::frame::ETX})));

TEST(frameDecoderTest, AddMoreThanMax) {
  const size_t N = 5;
  kodebooth::frame::Decoder<N> decoder(
      kodebooth::frame::IntegrityCheck::Without);
  {
    auto [buffer, size] = decoder.put(kodebooth::frame::STX);
    EXPECT_FALSE(buffer);
    EXPECT_EQ(size, 1);
  }
  for (int i = 0; i < N; i++) {
    auto [buffer, size] = decoder.put(NON_SPECIAL_VALUE);
    EXPECT_FALSE(buffer);
    EXPECT_EQ(size, 1);
  }

  {
    auto [buffer, size] = decoder.put(NON_SPECIAL_VALUE);
    EXPECT_FALSE(buffer);
    EXPECT_EQ(size, 0);
  }

  auto [buffer, size] = decoder.put(kodebooth::frame::ETX);
  EXPECT_TRUE(buffer);
  EXPECT_EQ(size, N);
}