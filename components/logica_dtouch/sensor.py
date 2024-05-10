import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor
from esphome.const import (
    CONF_ID,
    CONF_NAME,
    CONF_ADDRESS,
    CONF_TEMPERATURE,
    CONF_UPDATE_INTERVAL,
    STATE_CLASS_MEASUREMENT,
    UNIT_CELSIUS,
    UNIT_PERCENT,
    DEVICE_CLASS_TEMPERATURE,
    DEVICE_CLASS_MOISTURE,
    DEVICE_CLASS_SPEED,
    DEVICE_CLASS_WATER,
    DEVICE_CLASS_WIND_SPEED
)

CONF_MOISTURE_CONTENT = "moisture_content"
CONF_EQUILIBRIUM_MOISTURE_CONTENT = "equilibrium_moisture_content"
CONF_HEATING_LEVEL = "heating_level"
CONF_FANS_LEVEL = "fans_level"
CONF_FLAPS_LEVEL = "flaps_level"
CONF_SPRAYER_LEVEL = "sprayer_level"
CONF_NUM_PROBES = "num_probes"
CONF_IDEAL_SENSOR = "report_ideal_value"
CONF_FINAL_SENSOR = "report_final_value"

DEPENDENCIES = ["uart"]

logica_dtouch_ns = cg.esphome_ns.namespace("logica_dtouch")
LOGICA_dTouch = logica_dtouch_ns.class_("LOGICA_dTouch", cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(LOGICA_dTouch),
            cv.Optional(CONF_TEMPERATURE): sensor.sensor_schema(
                unit_of_measurement=UNIT_CELSIUS,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(cv.Schema({
                cv.Required(CONF_NUM_PROBES): cv.int_range(min=0),
                cv.Optional(CONF_IDEAL_SENSOR, default=False): cv.boolean,
                cv.Optional(CONF_FINAL_SENSOR, default=False): cv.boolean,
            })),
            cv.Optional(CONF_MOISTURE_CONTENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_MOISTURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(cv.Schema({
                cv.Required(CONF_NUM_PROBES): cv.int_range(min=0),
                cv.Optional(CONF_FINAL_SENSOR, default=False): cv.boolean,
            })),
            cv.Optional(CONF_EQUILIBRIUM_MOISTURE_CONTENT): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_MOISTURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ).extend(cv.Schema({
                cv.Required(CONF_NUM_PROBES): cv.int_range(min=0),
                cv.Optional(CONF_IDEAL_SENSOR, default=False): cv.boolean,
                cv.Optional(CONF_FINAL_SENSOR, default=False): cv.boolean,
            })),
            cv.Optional(CONF_HEATING_LEVEL): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_TEMPERATURE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_FANS_LEVEL): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_SPEED,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_FLAPS_LEVEL): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_WIND_SPEED,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_SPRAYER_LEVEL): sensor.sensor_schema(
                unit_of_measurement=UNIT_PERCENT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_WATER,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_ADDRESS, default=1): cv.int_range(min=1, max=254),
            cv.Optional(CONF_UPDATE_INTERVAL, default="5s"): cv.update_interval,
        }
    )
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if CONF_MOISTURE_CONTENT in config:
        sens = await sensor.new_sensor(config[CONF_MOISTURE_CONTENT])
        cg.add(var.set_mc_sensor(sens))
        cg.add(var.add_command())
        for idx in range(0, config[CONF_MOISTURE_CONTENT][CONF_NUM_PROBES]):
            custom_conf = config[CONF_MOISTURE_CONTENT].copy()
            custom_conf[CONF_ID] = custom_conf[CONF_ID].copy()
            custom_conf[CONF_ID].id = custom_conf[CONF_ID].id + "_probe_" + str(idx+1)
            custom_conf[CONF_NAME] = config[CONF_MOISTURE_CONTENT][CONF_NAME] + "_probe_" + str(idx+1)
            sens = await sensor.new_sensor(custom_conf)
            cg.add(var.add_mc_probe(sens))
        
        if config[CONF_MOISTURE_CONTENT][CONF_FINAL_SENSOR]:
            custom_conf = config[CONF_MOISTURE_CONTENT].copy()
            custom_conf[CONF_ID] = custom_conf[CONF_ID].copy()
            custom_conf[CONF_ID].id = custom_conf[CONF_ID].id + "_final"
            custom_conf[CONF_NAME] = config[CONF_MOISTURE_CONTENT][CONF_NAME] + "_final"
            sens = await sensor.new_sensor(custom_conf)
            cg.add(var.set_mc_sensor_final(sens))

    if CONF_EQUILIBRIUM_MOISTURE_CONTENT in config:
        sens = await sensor.new_sensor(config[CONF_EQUILIBRIUM_MOISTURE_CONTENT])
        cg.add(var.set_emc_sensor(sens))
        cg.add(var.add_command())
        
        for idx in range(0, config[CONF_EQUILIBRIUM_MOISTURE_CONTENT][CONF_NUM_PROBES]):
            custom_conf = config[CONF_EQUILIBRIUM_MOISTURE_CONTENT].copy()
            custom_conf[CONF_ID] = custom_conf[CONF_ID].copy()
            custom_conf[CONF_ID].id = custom_conf[CONF_ID].id + "_probe_" + str(idx+1)
            custom_conf[CONF_NAME] = config[CONF_EQUILIBRIUM_MOISTURE_CONTENT][CONF_NAME] + "_probe_" + str(idx+1)
            sens = await sensor.new_sensor(custom_conf)
            cg.add(var.add_emc_probe(sens))
        
        if config[CONF_EQUILIBRIUM_MOISTURE_CONTENT][CONF_IDEAL_SENSOR]:
            custom_conf = config[CONF_EQUILIBRIUM_MOISTURE_CONTENT].copy()
            custom_conf[CONF_ID] = custom_conf[CONF_ID].copy()
            custom_conf[CONF_ID].id = custom_conf[CONF_ID].id + "_ideal"
            custom_conf[CONF_NAME] = config[CONF_EQUILIBRIUM_MOISTURE_CONTENT][CONF_NAME] + "_ideal"
            sens = await sensor.new_sensor(custom_conf)
            cg.add(var.set_emc_sensor_ideal(sens))
        
        if config[CONF_EQUILIBRIUM_MOISTURE_CONTENT][CONF_FINAL_SENSOR]:
            custom_conf = config[CONF_EQUILIBRIUM_MOISTURE_CONTENT].copy()
            custom_conf[CONF_ID] = custom_conf[CONF_ID].copy()
            custom_conf[CONF_ID].id = custom_conf[CONF_ID].id + "_final"
            custom_conf[CONF_NAME] = config[CONF_EQUILIBRIUM_MOISTURE_CONTENT][CONF_NAME] + "_final"
            sens = await sensor.new_sensor(custom_conf)
            cg.add(var.set_emc_sensor_final(sens))

    if CONF_TEMPERATURE in config:
        sens = await sensor.new_sensor(config[CONF_TEMPERATURE])
        cg.add(var.set_temperature_sensor(sens))
        cg.add(var.add_command())
        for idx in range(0, config[CONF_TEMPERATURE][CONF_NUM_PROBES]):
            custom_conf = config[CONF_TEMPERATURE].copy()
            custom_conf[CONF_ID] = custom_conf[CONF_ID].copy()
            custom_conf[CONF_ID].id = custom_conf[CONF_ID].id + "_probe_" + str(idx+1)
            custom_conf[CONF_NAME] = config[CONF_TEMPERATURE][CONF_NAME] + "_probe_" + str(idx+1)
            sens = await sensor.new_sensor(custom_conf)
            cg.add(var.add_temperature_probe(sens))
        
        if config[CONF_TEMPERATURE][CONF_IDEAL_SENSOR]:
            custom_conf = config[CONF_TEMPERATURE].copy()
            custom_conf[CONF_ID] = custom_conf[CONF_ID].copy()
            custom_conf[CONF_ID].id = custom_conf[CONF_ID].id + "_ideal"
            custom_conf[CONF_NAME] = config[CONF_TEMPERATURE][CONF_NAME] + "_ideal"
            sens = await sensor.new_sensor(custom_conf)
            cg.add(var.set_temperature_sensor_ideal(sens))
        
        if config[CONF_TEMPERATURE][CONF_FINAL_SENSOR]:
            custom_conf = config[CONF_TEMPERATURE].copy()
            custom_conf[CONF_ID] = custom_conf[CONF_ID].copy()
            custom_conf[CONF_ID].id = custom_conf[CONF_ID].id + "_final"
            custom_conf[CONF_NAME] = config[CONF_TEMPERATURE][CONF_NAME] + "_final"
            sens = await sensor.new_sensor(custom_conf)
            cg.add(var.set_temperature_sensor_final(sens))

    if CONF_HEATING_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_HEATING_LEVEL])
        cg.add(var.set_heating_sensor(sens))
    
    if CONF_FANS_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_FANS_LEVEL])
        cg.add(var.set_fans_sensor(sens))
    
    if CONF_FLAPS_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_FLAPS_LEVEL])
        cg.add(var.set_flaps_sensor(sens))
    
    if CONF_SPRAYER_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_SPRAYER_LEVEL])
        cg.add(var.set_sprayer_sensor(sens))

    if (
        config[CONF_TEMPERATURE][CONF_IDEAL_SENSOR] or
        config[CONF_TEMPERATURE][CONF_FINAL_SENSOR] or
        config[CONF_MOISTURE_CONTENT][CONF_FINAL_SENSOR] or
        config[CONF_EQUILIBRIUM_MOISTURE_CONTENT][CONF_IDEAL_SENSOR] or
        config[CONF_EQUILIBRIUM_MOISTURE_CONTENT][CONF_FINAL_SENSOR] or
        CONF_HEATING_LEVEL in config or
        CONF_FANS_LEVEL in config or
        CONF_FLAPS_LEVEL in config or
        CONF_SPRAYER_LEVEL in config
    ):
        var.add_command()

    cg.add(var.set_address(config[CONF_ADDRESS]))
    cg.add(var.set_update_interval(config[CONF_UPDATE_INTERVAL]))