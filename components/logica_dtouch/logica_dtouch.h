#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace logica_dtouch {

class LOGICA_dTouch : public PollingComponent, public uart::UARTDevice {
 public:
  float get_setup_priority() const override;

  void setup() override;
  void loop() override;
  void update() override;
  void dump_config() override;

  void set_temperature_sensor(sensor::Sensor *temperature_sensor) { temperature_sensor_ = temperature_sensor; }
  void set_emc_sensor(sensor::Sensor *emc_sensor) { emc_sensor_ = emc_sensor; }
  void set_mc_sensor(sensor::Sensor *mc_sensor) { mc_sensor_ = mc_sensor; }
  void set_address(uint32_t address) { address_ = address; }

 protected:
  void dtouch_send_command_(const uint8_t command);
  void dtouch_send_command_(const uint8_t command, const uint8_t *data, const size_t data_len);
  size_t dtouch_receive_packet_(uint8_t *response, const size_t response_len);

  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *emc_sensor_{nullptr};
  sensor::Sensor *mc_sensor_{nullptr};
  uint8_t address_;
};

}  // namespace logica_dtouch
}  // namespace esphome
