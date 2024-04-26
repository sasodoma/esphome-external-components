#include "logica_dtouch.h"
#include "esphome/core/log.h"

namespace esphome {
namespace logica_dtouch {

static const char *const TAG = "logica_dtouch";
static const size_t DTOUCH_S_RESPONSE_LENGTH = 113;
static const size_t DTOUCH_S_RESPONSE_TEMP_OFFSET = 77;
static const size_t DTOUCH_S_RESPONSE_MC_OFFSET = 73;
static const size_t DTOUCH_S_RESPONSE_EMC_OFFSET = 83;
static const size_t DTOUCH_HEADER_LENGTH = 4;
static const uint8_t DTOUCH_HEADER[] = { 0x01, 0x80, 0x00, 0x00 };

// The CRC used is CRC-16/MODBUS, sent low byte first
uint16_t dtouch_crc(const uint8_t *bytes, size_t len, bool restart) {
    static uint16_t crc = 0xFFFF;
    uint8_t i, j;

    if (restart)
      crc = 0xFFFF;

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

void LOGICA_dTouch::setup() {}

void LOGICA_dTouch::update() {

  uint8_t response[DTOUCH_S_RESPONSE_LENGTH];
  if (!this->dtouch_send_command_('S', response, DTOUCH_S_RESPONSE_LENGTH)) {
    ESP_LOGW(TAG, "Reading data from dTouch failed!");
    return;
  }

  if (response[0] != this->address_ || response[1] != 0x80) {
    ESP_LOGW(TAG, "Invalid header from dTouch!");
    return;
  }

  uint16_t checksum = dtouch_crc(response, DTOUCH_S_RESPONSE_LENGTH - 2, true);
  if (response[DTOUCH_S_RESPONSE_LENGTH - 2] != checksum & 0xFF || response[DTOUCH_S_RESPONSE_LENGTH - 1] != checksum >> 8) {
    ESP_LOGW(TAG, "dTouch checksum doesn't match: 0x%02X%02X!=0x%04X", response[DTOUCH_S_RESPONSE_LENGTH - 2], response[DTOUCH_S_RESPONSE_LENGTH - 1], checksum);
    return;
  }

  this->status_clear_warning();
  const float temp = ((response[DTOUCH_S_RESPONSE_TEMP_OFFSET] << 8) | response[DTOUCH_S_RESPONSE_TEMP_OFFSET+1]) / 10.0f;
  const float emc = ((response[DTOUCH_S_RESPONSE_EMC_OFFSET] << 8) | response[DTOUCH_S_RESPONSE_EMC_OFFSET+1]) / 10.0f;
  const float mc = ((response[DTOUCH_S_RESPONSE_MC_OFFSET] << 8) | response[DTOUCH_S_RESPONSE_MC_OFFSET+1]) / 10.0f;


  ESP_LOGD(TAG, "dTouch Received Temperature=%f°C, EMC=%f%%, MC=%f%%", temp, emc, mc);
  if (this->temperature_sensor_ != nullptr)
    this->temperature_sensor_->publish_state(temp);
  if (this->emc_sensor_ != nullptr)
    this->emc_sensor_->publish_state(emc);
  if (this->mc_sensor_ != nullptr)
    this->mc_sensor_->publish_state(mc);
}

bool LOGICA_dTouch::dtouch_send_command_(const uint8_t command, uint8_t *response, const size_t response_len) {
    return dtouch_send_command_data_(command, nullptr, 0, response, response_len);
}

bool LOGICA_dTouch::dtouch_send_command_data_(const uint8_t command, const uint8_t *data, const size_t data_len, uint8_t *response, const size_t response_len) {
  // Empty RX Buffer
  while (this->available())
    this->read();
  
  this->write_array(DTOUCH_HEADER, DTOUCH_HEADER_LENGTH);
  dtouch_crc(DTOUCH_HEADER, DTOUCH_HEADER_LENGTH, true);
  uint16_t payload_length = data_len + 1;
  uint8_t length_cmd[] = {((uint16_t) payload_length & 0xFF00) >> 8, (uint8_t) payload_length, command};
  this->write_array(length_cmd, 3);
  dtouch_crc(length_cmd, 3, false);
  if (data != nullptr) {
    this->write_array(data, data_len);
    dtouch_crc(data, data_len, false);
  }
  uint16_t crc = dtouch_crc(nullptr, 0, false);
  uint8_t crc_bytes[] = {crc & 0xFF, crc >> 8};
  this->write_array(crc_bytes, 2);
  this->flush();

  if (response == nullptr)
    return true;

  return this->read_array(response, response_len);
}

float LOGICA_dTouch::get_setup_priority() const { return setup_priority::DATA; }

void LOGICA_dTouch::dump_config() {
  ESP_LOGCONFIG(TAG, "Logica dTouch:");
  LOG_SENSOR("  ", "Temperautre", this->temperature_sensor_);
  LOG_SENSOR("  ", "MC", this->mc_sensor_);
  LOG_SENSOR("  ", "EMC", this->emc_sensor_);
  this->check_uart_settings(57600, 1, uart::UART_CONFIG_PARITY_EVEN, 8);

  ESP_LOGCONFIG(TAG, "  Device address: %d", this->address_);
}

}  // namespace logica_dtouch
}  // namespace esphome
