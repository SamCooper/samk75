#define DT_DRV_COMPAT issi_is31fl3731

#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/led.h>
#include <zephyr/logging/log.h>

#define ISSI_ADDR_DEFAULT 0x74

#define ISSI_REG_CONFIG 0x00
#define ISSI_REG_CONFIG_PICTUREMODE 0x00
#define ISSI_REG_CONFIG_AUTOPLAYMODE 0x08
#define ISSI_REG_CONFIG_AUDIOPLAYMODE 0x18

#define ISSI_CONF_PICTUREMODE 0x00
#define ISSI_CONF_AUTOFRAMEMODE 0x04
#define ISSI_CONF_AUDIOMODE 0x08

#define ISSI_REG_PICTUREFRAME 0x01

#define ISSI_REG_SHUTDOWN 0x0A
#define ISSI_REG_AUDIOSYNC 0x06

#define ISSI_REG_LED_FIRST 0x24

#define ISSI_COMMANDREGISTER 0xFD
#define ISSI_BANK_FUNCTIONREG 0x0B // helpfully called 'page nine'

#define IS31FL3731_MAX_LEDS		144

LOG_MODULE_REGISTER(is31fl3731, CONFIG_LED_LOG_LEVEL);

struct is31fl3731_cfg {
	struct i2c_dt_spec i2c;
};


static int is31fl3731_write_buffer(const struct i2c_dt_spec *i2c,
				    const uint8_t *buffer, uint32_t num_bytes)
{
	int status;

	status = i2c_write_dt(i2c, buffer, num_bytes);
	if (status < 0) {
		LOG_ERR("Could not write buffer: %i", status);
		return status;
	}

	return 0;
}

static int is31fl3731_write_reg(const struct i2c_dt_spec *i2c, uint8_t reg, uint8_t val)
{
	uint8_t buffer[2] = {reg, val};

	int status = is31fl3731_write_buffer(i2c, buffer, sizeof(buffer));
	LOG_ERR("IS31FL3731 REG:%d to %d  returned: %d", reg, val, status);
    return status;
}

//static int is31fl3731_write_reg8(const struct i2c_dt_spec *i2c, uint8_t bank, uint8_t reg, uint8_t data) {
//	int status;
//
//	status = is31fl3731_write_reg(i2c, ISSI_COMMANDREGISTER, bank);
//	if (status < 0) {
//		LOG_ERR("Could not write buffer: %i", status);
//		return status;
//	}
//
//    return is31fl3731_write_reg(i2c, reg, data);
//}
//

static int is31fl3731_init_registers(const struct i2c_dt_spec *i2c)
{
  is31fl3731_write_reg(i2c, ISSI_COMMANDREGISTER, ISSI_BANK_FUNCTIONREG);
  // shutdown
  is31fl3731_write_reg(i2c, ISSI_REG_SHUTDOWN, 0x00);

  k_msleep(10);

  // out of shutdown
  is31fl3731_write_reg(i2c, ISSI_REG_SHUTDOWN, 0x01);
  is31fl3731_write_reg(i2c, ISSI_REG_CONFIG, ISSI_REG_CONFIG_PICTUREMODE);
  is31fl3731_write_reg(i2c, ISSI_REG_PICTUREFRAME, 0);
  is31fl3731_write_reg(i2c, ISSI_REG_AUDIOSYNC, 0x0);

  // all LEDs on & 0 PWM
  uint8_t erasebuf[25];
  memset(erasebuf, 0, 25);

  // set each led to 0 PWM
  is31fl3731_write_reg(i2c, ISSI_COMMANDREGISTER, 0);
  for (uint8_t i = 0; i < 6; i++) {
    erasebuf[0] = 0x24 + i * 24;
    is31fl3731_write_buffer(i2c, erasebuf, 25);
  }

  for (uint8_t i = 0; i <= 0x11; i++) {
    is31fl3731_write_reg(i2c, i, 0xff);  // each 8 LEDs on
  }

  return 0;
}

static int is31fl3731_led_set_brightness(const struct device *dev, uint32_t led, uint8_t value)
{
	const struct is31fl3731_cfg *config = dev->config;
	LOG_ERR("IS31FL3731 set brightness LED:%d to %d", led, value);

	//is31fl3731_init_registers(&config->i2c);
	return is31fl3731_write_reg(&config->i2c, 0x24 + 59, 255);

//	const struct is31fl3731_cfg *config = dev->config;
//	uint8_t pwm_reg = ISSI_REG_LED_FIRST + led;
//
//	if (led > IS31FL3731_MAX_LEDS - 1) {
//		return -EINVAL;
//	}
//
//	return is31fl3731_write_reg8(&config->i2c, 0, pwm_reg, value);
}

static int is31fl3731_led_on(const struct device *dev, uint32_t led) {
	return 0;//is31fl3731_led_set_brightness(dev, led, 100);
}

static int is31fl3731_led_off(const struct device *dev, uint32_t led) {
	return 0;//is31fl3731_led_set_brightness(dev, led, 0);
}

static int is31fl3731_init(const struct device *dev)
{
	const struct is31fl3731_cfg *config = dev->config;

	LOG_DBG("Initializing @0x%x...", config->i2c.addr);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C device not ready");
		return -ENODEV;
	}

	return is31fl3731_init_registers(&config->i2c);
}

static const struct led_driver_api is31fl3731_led_api = {
	.set_brightness = is31fl3731_led_set_brightness,
	.on = is31fl3731_led_on,
	.off = is31fl3731_led_off
};

#define IS31FL3731_INIT(id) \
	static const struct is31fl3731_cfg is31fl3731_##id##_cfg = {	\
		.i2c = I2C_DT_SPEC_INST_GET(id),			\
	};								\
	DEVICE_DT_INST_DEFINE(id, &is31fl3731_init, NULL, NULL,	\
		&is31fl3731_##id##_cfg, POST_KERNEL,			\
		CONFIG_LED_INIT_PRIORITY, &is31fl3731_led_api);

DT_INST_FOREACH_STATUS_OKAY(IS31FL3731_INIT)

