#include <gtest/gtest.h>

#include <vector>

#include "esphome/components/cst328/touchscreen/cst328_touchscreen.h"
#include "../common.h"

namespace esphome::cst328::testing {

class TestableCST328 : public CST328Touchscreen {
 public:
  using CST328Touchscreen::setup_complete_;
  using CST328Touchscreen::update_touches;
  using CST328Touchscreen::x_raw_max_;
  using CST328Touchscreen::y_raw_max_;
};

TEST(CST328TouchscreenTest, SetupReadsTheResolution) {
  RegisterBus bus;
  bus.registers[CST_REG_FW_CRC_AND_BOOT_TIME] = {0, 0, 0xCA, 0xCA};
  bus.registers[CST_REG_X_Y_RESOLUTION] = {0x40, 0x01, 0xF0, 0x00};
  TestableCST328 hub;
  hub.set_i2c_bus(&bus);
  hub.setup();
  EXPECT_TRUE(hub.setup_complete_);
  EXPECT_EQ(hub.x_raw_max_, 320);
  EXPECT_EQ(hub.y_raw_max_, 240);
}

TEST(CST328TouchscreenTest, SetupFailsOnAFirmwareCrcMismatch) {
  RegisterBus bus;
  TestableCST328 hub;
  hub.set_i2c_bus(&bus);
  hub.setup();
  EXPECT_FALSE(hub.setup_complete_);
  EXPECT_TRUE(hub.is_failed());
}

class RecordingListener final : public CST328ButtonListener {
 public:
  void update_button(bool state) override { this->states.push_back(state); }
  std::vector<bool> states;
};

class CST3240Test : public ::testing::Test {
 protected:
  void SetUp() override {
    this->bus_.registers[CST_REG_X_Y_RESOLUTION] = {0x40, 0x01, 0xF0, 0x00};
    this->hub_.set_cst3240(true);
    this->hub_.set_i2c_bus(&this->bus_);
    this->hub_.register_button_listener(&this->listener_);
    this->hub_.setup();
  }

  RegisterBus bus_;
  TestableCST328 hub_;
  RecordingListener listener_;
};

TEST_F(CST3240Test, SetupSkipsTheFirmwareCrc) {
  EXPECT_TRUE(this->hub_.setup_complete_);
  EXPECT_FALSE(this->hub_.is_failed());
  EXPECT_EQ(this->hub_.x_raw_max_, 320);
}

TEST_F(CST3240Test, ReportWithoutTheSyncByteIsSkippedAndCleared) {
  this->bus_.set_finger_count(1, 0x55);
  this->hub_.update_touches();
  EXPECT_TRUE(this->listener_.states.empty());
  EXPECT_EQ(this->bus_.last_write, CST_REG_TOUCH_INFORMATION);
  EXPECT_EQ(this->bus_.last_write_size, 2u + CST3240_TOUCH_DATA_SIZE);
}

TEST_F(CST3240Test, IdleBlockIsNotRewritten) {
  this->bus_.set_finger_count(0);
  this->bus_.last_write_size = 0;
  this->hub_.update_touches();
  EXPECT_EQ(this->bus_.last_write_size, 0u);
}

TEST_F(CST3240Test, ReleasedRecordReleasesTheTouch) {
  this->bus_.set_finger_count(1, 0xAB);
  this->hub_.update_touches();
  this->bus_.set_finger_count(1, 0xAB, 0);  // the release report still lists the finger
  this->hub_.update_touches();
  this->bus_.set_finger_count(0);
  this->hub_.update_touches();
  EXPECT_EQ(this->listener_.states, (std::vector<bool>{true, false}));
}

TEST_F(CST3240Test, AtMostThreeFingers) {
  this->bus_.set_finger_count(3, 0xAB);
  this->hub_.update_touches();
  this->bus_.set_finger_count(4, 0xAB);
  this->hub_.update_touches();
  EXPECT_EQ(this->listener_.states, (std::vector<bool>{true, false}));
}

TEST_F(CST3240Test, PollingClearsTheTouchBlock) {
  this->bus_.set_finger_count(1, 0xAB);
  this->hub_.update_touches();
  EXPECT_EQ(this->bus_.last_write, CST_REG_TOUCH_INFORMATION);
  EXPECT_EQ(this->bus_.last_write_size, 2u + CST3240_TOUCH_DATA_SIZE);
}

}  // namespace esphome::cst328::testing
