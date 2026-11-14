#pragma once

#include <algorithm>
#include <cstring>
#include <map>
#include <vector>

#include "esphome/core/gpio.h"
#include "esphome/components/cst328/touchscreen/cst328_touchscreen.h"
#include "esphome/core/helpers.h"

namespace esphome::cst328::testing {

// A reset pin that only records its level.
class FakePin final : public GPIOPin {
 public:
  void setup() override {}
  void pin_mode(gpio::Flags flags) override {}
  gpio::Flags get_flags() const override { return gpio::FLAG_OUTPUT; }
  bool digital_read() override { return this->level; }
  void digital_write(bool value) override { this->level = value; }
  bool level{false};
};

// Answers 16-bit register reads from a map; every write succeeds and records its register.
class RegisterBus final : public i2c::I2CBus {
 public:
  std::map<uint16_t, std::vector<uint8_t>> registers;
  uint16_t last_write{0};
  size_t last_write_size{0};

  i2c::ErrorCode write_readv(uint8_t address, const uint8_t *data, size_t size, uint8_t *out,
                             size_t out_size) override {
    if (out_size == 0) {
      if (size >= 2)
        this->last_write = encode_uint16(data[0], data[1]);
      this->last_write_size = size;
      return i2c::ERROR_OK;
    }
    if (size < 2)
      return i2c::ERROR_NOT_ACKNOWLEDGED;
    auto &value = this->registers[encode_uint16(data[0], data[1])];
    value.resize(std::max(value.size(), out_size));
    std::memcpy(out, value.data(), out_size);
    return i2c::ERROR_OK;
  }

  /// event 0x06 marks each record pressed; 0 marks it released.
  void set_finger_count(uint8_t count, uint8_t sync = 0, uint8_t event = 0x06) {
    std::vector<uint8_t> data(CST328_TOUCH_DATA_SIZE);
    for (size_t i = 0, idx = 0; i < count && idx < data.size(); idx += (i == 0) ? 7 : 5, i++)
      data[idx] = event;
    data[CST_REG_FINGER_COUNT_IDX] = count;
    data[CST_REG_SYNC_IDX] = sync;
    this->registers[CST_REG_TOUCH_INFORMATION] = data;
  }
};

}  // namespace esphome::cst328::testing
