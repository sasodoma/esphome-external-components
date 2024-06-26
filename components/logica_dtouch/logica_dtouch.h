#pragma once

#include <vector>
#include <queue>
#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/uart/uart.h"

namespace esphome {
namespace logica_dtouch {

class LOGICA_dTouch : public Component, public uart::UARTDevice {
 public:
  float get_setup_priority() const override;

  void setup() override;
  void loop() override;
  void dump_config() override;
  // Implement our own update with custom timing
  void update();

  void set_temperature_sensor(sensor::Sensor *sensor) { temperature_sensor_ = sensor; }
  void set_temperature_sensor_ideal(sensor::Sensor *sensor) { temperature_sensor_ideal_ = sensor; }
  void set_temperature_sensor_final(sensor::Sensor *sensor) { temperature_sensor_final_ = sensor; }
  void add_temperature_probe(sensor::Sensor *probe) { temperature_probes_.push_back(probe); }

  void set_emc_sensor(sensor::Sensor *sensor) { emc_sensor_ = sensor; }
  void set_emc_sensor_ideal(sensor::Sensor *sensor) { emc_sensor_ideal_ = sensor; }
  void set_emc_sensor_final(sensor::Sensor *sensor) { emc_sensor_final_ = sensor; }
  void add_emc_probe(sensor::Sensor *probe) { emc_probes_.push_back(probe); }

  void set_mc_sensor(sensor::Sensor *sensor) { mc_sensor_ = sensor; }
  void set_mc_sensor_final(sensor::Sensor *sensor) { mc_sensor_final_ = sensor; }
  void add_mc_probe(sensor::Sensor *probe) { mc_probes_.push_back(probe); }

  void set_heating_sensor(sensor::Sensor *sensor) { heating_sensor_ = sensor; }
  void set_fans_sensor(sensor::Sensor *sensor) {fans_sensor_ = sensor; }
  void set_flaps_sensor(sensor::Sensor *sensor) { flaps_sensor_ = sensor; }
  void set_sprayer_sensor(sensor::Sensor *sensor) { sprayer_sensor_ = sensor; }

  void set_address(uint32_t address) { address_ = address; }
  void add_command() { command_num_++; }
  void set_update_interval(uint32_t update_interval);

 protected:
  void dtouch_send_command_(const uint8_t command) { dtouch_send_command_(command, nullptr, 0); }
  void dtouch_send_command_(const uint8_t command, const uint8_t *data, const size_t data_len);
  size_t dtouch_receive_packet_(uint8_t *response, const size_t response_len);

  void dtouch_parse_packet_P_00_(const uint8_t *data, const size_t len);
  void dtouch_parse_packet_P_02_(const uint8_t *data, const size_t len);
  void dtouch_parse_packet_P_03_(const uint8_t *data, const size_t len);
  void dtouch_parse_packet_P_10_(const uint8_t *data, const size_t len);

  sensor::Sensor *temperature_sensor_{nullptr};
  sensor::Sensor *temperature_sensor_ideal_{nullptr};
  sensor::Sensor *temperature_sensor_final_{nullptr};
  std::vector<sensor::Sensor *> temperature_probes_;

  sensor::Sensor *emc_sensor_{nullptr};
  sensor::Sensor *emc_sensor_ideal_{nullptr};
  sensor::Sensor *emc_sensor_final_{nullptr};
  std::vector<sensor::Sensor *> emc_probes_;

  sensor::Sensor *mc_sensor_{nullptr};
  sensor::Sensor *mc_sensor_final_{nullptr};
  std::vector<sensor::Sensor *> mc_probes_;

  sensor::Sensor *heating_sensor_{nullptr};
  sensor::Sensor *fans_sensor_{nullptr};
  sensor::Sensor *flaps_sensor_{nullptr};
  sensor::Sensor *sprayer_sensor_{nullptr};

  struct sensor_update {
    sensor::Sensor *sensor;
    float value;
  };

  std::queue<sensor_update> publish_queue_;

  uint8_t address_;
  uint32_t update_interval_;
  unsigned int command_num_ = 0;
  bool use_command_control_values_;

  struct {
    uint8_t command = 0;
    uint8_t data = 0;
    unsigned long time = 0;
  } last_sent_command_;
};

}  // namespace logica_dtouch
}  // namespace esphome
