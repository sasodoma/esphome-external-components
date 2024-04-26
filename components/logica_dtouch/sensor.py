import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor
from esphome.const import (
    CONF_ID,
    CONF_ADDRESS,
    CONF_TEMPERATURE,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_MOISTURE
)

CONF_MOISTURE_CONTENT = "moisture_content"
CONF_EQUIVALENT_MOISTURE_CONTENT = "equivalent_moisture_content"

DEPENDENCIES = ["uart"]

logica_dtouch_ns = cg.esphome_ns.namespace("logica_dtouch")
LOGICA_dTouch = logica_dtouch_ns.class_("LOGICA_dTouch", cg.PollingComponent, uart.UARTDevice)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LOGICA_dTouch),
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_MOISTURE_CONTENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_MOISTURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_EQUIVALENT_MOISTURE_CONTENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_MOISTURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_ADDRESS, default=1): cv.int_range(min=1, max=32),
        }
    )
    .extend(cv.polling_component_schema("5s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature_sensor(sens))

    if CONF_MOISTURE_CONTENT in config:
        sens = await sensor.new_sensor(config[CONF_MOISTURE_CONTENT])
        cg.add(var.set_mc_sensor(sens))

    if CONF_EQUIVALENT_MOISTURE_CONTENT in config:
        sens = await sensor.new_sensor(config[CONF_EQUIVALENT_MOISTURE_CONTENT])
        cg.add(var.set_emc_sensor(sens))

    cg.add(var.set_address(config[CONF_ADDRESS]))