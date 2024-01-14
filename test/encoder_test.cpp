// SPDX-License-Identifier: MIT

#include <gtest/gtest.h>
#include <vector>

#include "kodebooth/frame/frame.h"

static constexpr uint8_t NON_SPECIAL_VALUE = 42;
static_assert(NON_SPECIAL_VALUE != kodebooth::frame::STX);
static_assert(NON_SPECIAL_VALUE != kodebooth::frame::ETX);
static_assert(NON_SPECIAL_VALUE != kodebooth::frame::ESC);

class frameEncoderTest
    : public ::testing::TestWithParam<
          std::pair<std::vector<uint8_t>, std::vector<uint8_t>>> {};

TEST_P(frameEncoderTest, EncodeFrame) {
  auto [values, expected] = GetParam();

  kodebooth::frame::Encoder<100> encoder(
      kodebooth::frame::IntegrityCheck::Without);

  for (auto value : values) {
    EXPECT_EQ(encoder.put(value), 1);
  }

  auto [buffer, size] = encoder.finalize().value();
  EXPECT_EQ(size, expected.size());
  for (int i = 0; i < size; i++) {
    EXPECT_EQ(buffer[i], expected[i]);
  }
}

INSTANTIATE_TEST_SUITE_P(
    frameEncoderTests, frameEncoderTest,
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

TEST(frameEncoderTest, AddMoreThanMax) {
  const size_t N = 5;
  kodebooth::frame::Encoder<N, 2 + N> encoder(
      kodebooth::frame::IntegrityCheck::Without);
  for (int i = 0; i < N; i++) {
    EXPECT_EQ(encoder.put(NON_SPECIAL_VALUE), 1);
  }

  EXPECT_EQ(encoder.put(NON_SPECIAL_VALUE), 0);
}