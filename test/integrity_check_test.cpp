// SPDX-License-Identifier: MIT

#include <array>
#include <gtest/gtest.h>

#include "kodebooth/frame/frame.h"

TEST(frameIntegrityCheckTest, Valid) {
  kodebooth::frame::Encoder<100> encoder;
  kodebooth::frame::Decoder<100> decoder;

  std::array<uint8_t, 6> frame{'F', 'O', 'O', 'B', 'A', 'R'};

  encoder.put(frame);
  auto [encoded_buffer, encoded_length] = encoder.finalize().value();

  auto [decoded_buffer, decoded_length] =
      decoder.put(encoded_buffer, encoded_length);

  EXPECT_EQ(decoded_length, 6);
  EXPECT_TRUE(decoded_buffer.has_value());
  for (auto offset = 0; offset < 6; offset++)
    EXPECT_EQ(frame[offset], decoded_buffer.value()[offset]);
}

TEST(frameIntegrityCheckTest, Invalid) {
  kodebooth::frame::Encoder<100> encoder;
  kodebooth::frame::Decoder<100> decoder;

  std::array<uint8_t, 6> frame{'F', 'O', 'O', 'B', 'A', 'R'};

  encoder.put(frame);
  auto [encoded_buffer, encoded_length] = encoder.finalize().value();

  // corrupt frame
  encoded_buffer[1]++;

  auto [decoded_buffer, decoded_length] =
      decoder.put(encoded_buffer, encoded_length);

  EXPECT_EQ(decoded_length, encoded_length);
  EXPECT_FALSE(decoded_buffer.has_value());
}