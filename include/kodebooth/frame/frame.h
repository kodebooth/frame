// SPDX-License-Identifier: MIT

#ifndef KODEBOOTH_FRAME_FRAME_H
#define KODEBOOTH_FRAME_FRAME_H

#include <array>
#include <bit>
#include <cstdint>
#include <optional>
#include <utility>

#include "kodebooth/crc/crc.h"

namespace kodebooth {
namespace frame {

static constexpr uint8_t STX = 0x02;
static constexpr uint8_t ETX = 0x03;
static constexpr uint8_t ESC = 0x1b;
static constexpr size_t CRC_LENGTH = sizeof(uint32_t);

enum class IntegrityCheck {
  With,
  Without,
};

template <std::size_t N,
          std::size_t M = sizeof(STX) + 2 * (N + CRC_LENGTH) + sizeof(ETX)>
class Encoder {
private:
  std::array<uint8_t, M> buffer_{};
  std::size_t index_{};
  uint32_t crc32_{CRC_START_32};
  IntegrityCheck integrity_check_{};

  bool can_fit(std::size_t count) const {
    return index_ + count <= buffer_.size() - 1;
  }

  bool do_put(uint8_t value) {
    std::size_t needed_room = 1;

    if (needs_escape(value))
      needed_room++;

    if (!can_fit(needed_room)) {
      return false;
    }

    if (needed_room == 1) {
      buffer_[index_++] = value;
    } else {
      buffer_[index_++] = ESC;
      buffer_[index_++] = ~value;
    }

    return true;
  }

  static bool needs_escape(uint8_t value) {
    switch (value) {
    case STX:
    case ETX:
    case ESC:
      return true;
    }
    return false;
  }

public:
  explicit Encoder(IntegrityCheck integrity_check = IntegrityCheck::With)
      : integrity_check_(integrity_check) {
    reset();
  }

  void reset() {
    index_ = {};
    crc32_ = {CRC_START_32};
    buffer_[index_++] = STX;
  }

  template <size_t A> std::size_t put(const std::array<uint8_t, A> &values) {
    return put(values.data(), A);
  }

  std::size_t put(const uint8_t &value) { return put(&value, sizeof(value)); }

  std::size_t put(const uint8_t *values, std::size_t length) {
    for (auto offset = 0; offset < length; offset++) {
      auto value = values[offset];
      if (!do_put(value)) {
        return offset;
      }

      if (integrity_check_ == IntegrityCheck::With)
        crc32_ = kodebooth::crc::update_crc_32(crc32_, value);
    }

    return length;
  }

  std::optional<std::pair<uint8_t *, std::size_t>> finalize() {
    if (integrity_check_ == IntegrityCheck::With) {
      const auto big_endian_crc32 = std::endian::native == std::endian::big
                                        ? crc32_
                                        : std::byteswap(crc32_);
      auto ptr = reinterpret_cast<const uint8_t *>(&big_endian_crc32);
      for (auto offset = 0; offset < CRC_LENGTH; offset++) {
        if (!do_put(ptr[offset])) {
          return std::optional<std::pair<uint8_t *, std::size_t>>{};
        }
      }
    }
    buffer_[index_++] = ETX;
    return std::optional<std::pair<uint8_t *, std::size_t>>{
        std::make_pair(buffer_.data(), index_)};
  }
};

template <std::size_t N> class Decoder {
private:
  enum class State {
    WAIT_ON_STX,
    ACCEPTING,
    ESCAPED,
    VALID,
  };

  std::array<uint8_t, N> buffer_{};
  std::size_t index_{};
  State state{State::WAIT_ON_STX};
  IntegrityCheck integrity_check_{};

  bool validate() {
    if (integrity_check_ == IntegrityCheck::Without)
      return true;

    if (index_ < CRC_LENGTH)
      return false;

    uint32_t crc32;
    memcpy(&crc32, &buffer_[index_ - CRC_LENGTH], CRC_LENGTH);

    crc32 =
        std::endian::native == std::endian::big ? crc32 : std::byteswap(crc32);
    uint32_t calc_crc32 = {CRC_START_32};
    for (auto offset = 0; offset < index_ - CRC_LENGTH; offset++)
      calc_crc32 = kodebooth::crc::update_crc_32(calc_crc32, buffer_[offset]);

    return crc32 == calc_crc32;
  }

public:
  Decoder(IntegrityCheck integrity_check = IntegrityCheck::With)
      : integrity_check_(integrity_check) {
    reset();
  }

  void reset() {
    index_ = {};
    state = State::WAIT_ON_STX;
  }

  std::pair<std::optional<uint8_t *>, std::size_t> put(const uint8_t &value) {
    return put(&value, sizeof(value));
  }
  template <std::size_t A>
  std::size_t put(const std::array<uint8_t, A> &values) {
    return put(values.data(), A);
  }

  std::pair<std::optional<uint8_t *>, std::size_t> put(const uint8_t *values,
                                                       std::size_t length) {
    for (auto offset = 0; offset < length; offset++) {
      auto value = values[offset];

      switch (state) {
      case State::WAIT_ON_STX:
      default:
        switch (value) {
        case STX:
          state = State::ACCEPTING;
          break;
        default:
          break;
        }
        break;
      case State::ACCEPTING:
        switch (value) {
        case STX:
          index_ = {};
          break;
        case ETX:
          if (validate()) {
            auto length = integrity_check_ == IntegrityCheck::With
                              ? index_ - CRC_LENGTH
                              : index_;
            return std::make_pair(std::optional(buffer_.data()), length);
          }
          reset();
          break;
        case ESC:
          state = State::ESCAPED;
          break;
        default:
          if (index_ >= buffer_.size()) {
            return std::make_pair(std::optional<uint8_t *>{}, offset);
          }
          buffer_[index_++] = value;
          break;
        }
        break;
      case State::ESCAPED:
        switch (value) {
        case STX:
          index_ = {};
          state = State::ACCEPTING;
          break;
        case ETX:
        case ESC:
          reset();
          break;
        default:
          if (index_ >= buffer_.size())
            return std::make_pair(std::optional<uint8_t *>{}, offset);
          buffer_[index_++] = ~value;
          state = State::ACCEPTING;
          break;
        }
        break;
      }
    }

    return std::make_pair(std::optional<uint8_t *>{}, length);
  }
};
} // namespace frame
} // namespace kodebooth

#endif // KODEBOOTH_FRAME_FRAME_H