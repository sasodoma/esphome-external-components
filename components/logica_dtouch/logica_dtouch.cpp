#include "logica_dtouch.h"
#include "esphome/core/log.h"

#include <algorithm>

namespace esphome {
namespace logica_dtouch {

static const char *const TAG = "logica_dtouch";
static const size_t DTOUCH_MAX_RESPONSE_LENGTH = 128;
static const size_t DTOUCH_HEADER_LENGTH = 6;
static const size_t DTOUCH_STATIC_HEADER_LENGTH = 3;
static const uint8_t DTOUCH_STATIC_HEADER[] = { 0x80, 0x00, 0x00 };

enum SUPPORTED_COMMMANDS {
  COMMAND_MC,
  COMMAND_EMC,
  COMMAND_TEMPERATURE,
  COMMAND_CONTROL_VALUES
};

// The CRC used is CRC-16/MODBUS, sent low byte first
uint16_t dtouch_crc(const uint8_t *bytes, size_t len, uint16_t crc) {
  uint8_t i, j;

  for (i = 0; i < len; i++) {
      crc ^= bytes[i];
      for (j = 0; j < 8; j++) {
          if (crc & 0x0001) {
              crc >>= 1;
              crc ^= 0xA001;
          } else {
              crc >>= 1;
          }
      }
  }

  return crc;
}
uint16_t dtouch_crc(const uint8_t byte, uint16_t crc) { return dtouch_crc(&byte, 1, crc); }
uint16_t dtouch_crc(const uint8_t *bytes, size_t len) { return dtouch_crc(bytes, len, 0xFFFF); }
uint16_t dtouch_crc(const uint8_t byte) { return dtouch_crc(&byte, 1, 0xFFFF); }


void LOGICA_dTouch::setup() {
  this->use_command_control_values_ = (
    this->temperature_sensor_ideal_ != nullptr ||
    this->temperature_sensor_final_ != nullptr ||
    this->emc_sensor_ideal_ != nullptr ||
    this->emc_sensor_final_ != nullptr ||
    this->mc_sensor_final_ != nullptr ||
    this->heating_sensor_ != nullptr ||
    this->fans_sensor_ != nullptr ||
    this->flaps_sensor_ != nullptr ||
    this->sprayer_sensor_ != nullptr
  );
}

void LOGICA_dTouch::loop() {
  // Run the update loop
  if (update_interval_ == UINT32_MAX)
    return;
  static uint32_t last_update = 0;
  uint32_t time = millis();
  if (time - last_update > this->update_interval_) {
    this->update();
    last_update = time;
  }

  // Parse incoming data
  static uint8_t response[DTOUCH_MAX_RESPONSE_LENGTH];

  size_t received_length = this->dtouch_receive_packet_(response, DTOUCH_MAX_RESPONSE_LENGTH);

  if (!received_length)
    return;

  if (millis() - last_sent_command_.time > 500) {
    ESP_LOGW(TAG, "Received unsolicited packet");
    return;
  }

  uint16_t checksum = dtouch_crc(response, received_length - 2);
  if (response[received_length - 2] != (uint8_t) checksum || response[received_length - 1] != (uint8_t) (checksum >> 8)) {
    ESP_LOGW(TAG, "dTouch checksum doesn't match: 0x%02X%02X!=0x%04X", response[received_length - 1], response[received_length - 2], checksum);
    return;
  }

  switch (this->last_sent_command_.command) {
    case 'P':
      switch (this->last_sent_command_.data)
      {
      case 0x00:
        this->dtouch_parse_packet_P_00_(response, received_length);
        break;
      case 0x02:
        this->dtouch_parse_packet_P_02_(response, received_length);
        break;
      case 0x03:
        this->dtouch_parse_packet_P_03_(response, received_length);
        break;
      case 0x10:
        this->dtouch_parse_packet_P_10_(response, received_length);
        break;
      default:
        break;
      }
      break;
    default:
      break;
  }

  // Send one update from the queue, if any
  if (!this->publish_queue_.empty()) {
    sensor_update update = this->publish_queue_.front();
    this->publish_queue_.pop();
    update.sensor->publish_state(update.value);
  }
}

void LOGICA_dTouch::update() {
  static unsigned int current_command = 0;
  switch (current_command) {
    default:
      current_command = 0;
    case COMMAND_MC:
      current_command++;
      if (this->mc_sensor_ != nullptr) {
        const uint8_t data = 0x00;
        dtouch_send_command_('P', &data, 1);
        break;
      }
    case COMMAND_EMC:
      current_command++;
      if (this->emc_sensor_ != nullptr) {
        const uint8_t data = 0x02;
        dtouch_send_command_('P', &data, 1);
        break;
      }
    case COMMAND_TEMPERATURE:
      current_command++;
      if (this->temperature_sensor_ != nullptr) {
        const uint8_t data = 0x03;
        dtouch_send_command_('P', &data, 1);
        break;
      }
    case COMMAND_CONTROL_VALUES:
      current_command++;
      if (this->use_command_control_values_) {
        const uint8_t data = 0x10;
        dtouch_send_command_('P', &data, 1);
        break;
      }
  }
}

void LOGICA_dTouch::dtouch_send_command_(const uint8_t command, const uint8_t *data, const size_t data_len) {
  // Empty RX Buffer
  while (this->available())
    this->read();
  
  this->write_byte(this->address_);
  uint16_t crc = dtouch_crc(this->address_);
  this->write_array(DTOUCH_STATIC_HEADER, DTOUCH_STATIC_HEADER_LENGTH);
  crc = dtouch_crc(DTOUCH_STATIC_HEADER, DTOUCH_STATIC_HEADER_LENGTH, crc);
  uint16_t payload_length = data_len + 1;
  uint8_t length_cmd[] = {(uint8_t) (payload_length >> 8), (uint8_t) payload_length, command};
  this->write_array(length_cmd, 3);
  crc = dtouch_crc(length_cmd, 3, crc);
  if (data != nullptr) {
    this->write_array(data, data_len);
    crc = dtouch_crc(data, data_len, crc);
  }
  uint8_t crc_bytes[] = {(uint8_t) crc, uint8_t (crc >> 8)};
  this->write_array(crc_bytes, 2);
  this->flush();

  this->last_sent_command_.command = command;
  this->last_sent_command_.data = (data == nullptr) ? 0 : data[0];
  this->last_sent_command_.time = millis();
}

size_t LOGICA_dTouch::dtouch_receive_packet_(uint8_t *response, const size_t response_len) {
  static size_t current_index = 0;
  static size_t packet_length = 0;

  static unsigned long last_read = 0;

  while (this->available()) {
    last_read = millis();
    uint8_t data = this->read();
    if (current_index == 0 && data != this->address_)
      continue;
    if (current_index == 1 && data != 0x80) {
      current_index = 0;
      continue;
    }
    if (current_index == 4)
      packet_length = data << 8;
    if (current_index == 5) {
      packet_length += data + 8;
      if (packet_length > response_len) {
        ESP_LOGW(TAG, "Packet too long for array!");
        current_index = 0;
        continue;
      }
    }
    if (current_index == response_len) {
      current_index = 0;
      continue;
    }
    response[current_index++] = data;
    if (current_index == packet_length && current_index > 8) {
      current_index = 0;
      return packet_length;
    }
  }

  if (millis() - last_read > 10) {
    current_index = 0;
    last_read = millis();
  }
  return 0;
}

void LOGICA_dTouch::dtouch_parse_packet_P_00_(const uint8_t *data, const size_t len) {
  size_t index = DTOUCH_HEADER_LENGTH;
  const float total_mc = ((data[index] << 8) | data[index+1]) / 10.0f;
  this->publish_queue_.push({this->mc_sensor_, total_mc});
  index += 2;
  uint8_t num_probes = std::min(data[index++], (uint8_t) this->mc_probes_.size());
  for (int i = 0; i < num_probes; i++) {
    const float probe_mc = (((data[index] << 8) | data[index+1]) & 0xFFF) / 10.0f;
    this->publish_queue_.push({this->mc_probes_.at(i), probe_mc});
    index += 2;
  }
}

void LOGICA_dTouch::dtouch_parse_packet_P_02_(const uint8_t *data, const size_t len) {
  size_t index = DTOUCH_HEADER_LENGTH;
  const float total_emc = ((data[index] << 8) | data[index+1]) / 10.0f;
  this->publish_queue_.push({this->emc_sensor_, total_emc});
  index += 2;
  uint8_t num_probes = std::min(data[index++], (uint8_t) this->emc_probes_.size());
  for (int i = 0; i < num_probes; i++) {
    const float probe_emc = (((data[index] << 8) | data[index+1]) & 0xFFF) / 10.0f;
    this->publish_queue_.push({this->emc_probes_.at(i), probe_emc});
    index += 2;
  }
}

void LOGICA_dTouch::dtouch_parse_packet_P_03_(const uint8_t *data, const size_t len) {
  size_t index = DTOUCH_HEADER_LENGTH;
  const float total_temperature = ((data[index] << 8) | data[index+1]) / 10.0f;
  this->publish_queue_.push({this->temperature_sensor_, total_temperature});
  index += 2;
  uint8_t num_probes = std::min(data[index++], (uint8_t) this->temperature_probes_.size());
  for (int i = 0; i < num_probes; i++) {
    const float probe_temperature = (((data[index] << 8) | data[index+1]) & 0xFFF) / 10.0f;
    this->publish_queue_.push({this->temperature_probes_.at(i), probe_temperature});
    index += 2;
  }
}

void LOGICA_dTouch::dtouch_parse_packet_P_10_(const uint8_t *data, const size_t len) {
  size_t index = DTOUCH_HEADER_LENGTH;
  if (this->temperature_sensor_ideal_ != nullptr) {
    const float ideal_temperature = ((data[index] << 8) | data[index+1]) / 10.0f;
    this->publish_queue_.push({this->temperature_sensor_ideal_, ideal_temperature});
  }
  index += 2;
  if (this->temperature_sensor_final_ != nullptr) {
    const float final_temperature = ((data[index] << 8) | data[index+1]) / 10.0f;
    this->publish_queue_.push({this->temperature_sensor_final_, final_temperature});
  }
  index += 2;
  if (this->emc_sensor_ideal_ != nullptr) {
    const float ideal_emc = ((data[index] << 8) | data[index+1]) / 10.0f;
    this->publish_queue_.push({this->emc_sensor_ideal_, ideal_emc});
  }
  index += 2;
  if (this->emc_sensor_final_ != nullptr) {
    const float final_emc = ((data[index] << 8) | data[index+1]) / 10.0f;
    this->publish_queue_.push({this->emc_sensor_final_, final_emc});
  }
  index += 2;
  if (this->mc_sensor_final_ != nullptr) {
    const float final_mc = ((data[index] << 8) | data[index+1]) / 10.0f;
    this->publish_queue_.push({this->mc_sensor_final_, final_mc});
  }
  index += 2;
  index += 2;
  if (this->heating_sensor_ != nullptr) {
    const float heating_level = data[index];
    this->publish_queue_.push({this->heating_sensor_, heating_level});
  }
  index++;
  if (this->fans_sensor_ != nullptr) {
    const float fans_level = data[index];
    this->publish_queue_.push({this->fans_sensor_, fans_level});
  }
  index++;
  if (this->flaps_sensor_ != nullptr) {
    const float flaps_level = data[index];
    this->publish_queue_.push({this->flaps_sensor_, flaps_level});
  }
  index++;
  if (this->sprayer_sensor_ != nullptr) {
    const float sprayer_level = data[index];
    this->publish_queue_.push({this->sprayer_sensor_, sprayer_level});
  }
}

void LOGICA_dTouch::set_update_interval(uint32_t update_interval) {
  if (update_interval == UINT32_MAX) {
    this->update_interval_ = update_interval;
    return;
  }
  if (!this->command_num_) {
    this->update_interval_ = UINT32_MAX;
    return;
  }
  this->update_interval_ = (update_interval) / this->command_num_;
  if (this->update_interval_ < 500)
    this->update_interval_ = 500;
}

float LOGICA_dTouch::get_setup_priority() const { return setup_priority::DATA; }

void LOGICA_dTouch::dump_config() {
  ESP_LOGCONFIG(TAG, "Logica dTouch:");
  LOG_SENSOR("  ", "Temperautre", this->temperature_sensor_);
  if (this->temperature_probes_.size())
    ESP_LOGCONFIG(TAG, "    Temperature probes: %d", this->temperature_probes_.size());
  LOG_SENSOR("  ", "MC", this->mc_sensor_);
  if (this->mc_probes_.size())
    ESP_LOGCONFIG(TAG, "    MC probes: %d", this->mc_probes_.size());
  LOG_SENSOR("  ", "EMC", this->emc_sensor_);
  if (this->emc_probes_.size())
    ESP_LOGCONFIG(TAG, "    EMC probes: %d", this->emc_probes_.size());
  ESP_LOGCONFIG(TAG, "  Sending %d command(s), one every %u ms", this->command_num_, this->update_interval_);
  this->check_uart_settings(57600, 1, uart::UART_CONFIG_PARITY_EVEN, 8);

  ESP_LOGCONFIG(TAG, "  Device address: %d", this->address_);
}

}  // namespace logica_dtouch
}  // namespace esphome
