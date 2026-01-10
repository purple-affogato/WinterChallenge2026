#include "as7341.h"
#include "cyhal.h"

// const cyhal_i2c_cfg_t i2c_cfg = {
//     CYHAL_I2C_MODE_MASTER,
//     0,
//     100000u // clock frequency = 100 kHz
// };

// const uint8_t pon_data = 0b11;

// void initialize_as7341(AS7341* sensor) {
//     CY_ASSERT(sensor != NULL);
//     cy_rslt_t result;
//     result = cyhal_i2c_init(&(sensor->i2c_master), sensor->sda_pin, sensor->scl_pin, NULL);
//     result = cyhal_i2c_configure(&(sensor->i2c_master), &i2c_cfg);
//     result = cyhal_gpio_init(sensor->int_pin, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_PULLUP, true);
//     // result = cyhal_gpio_init(sensor->gpio_pin) // not sure if ill use this
//     sensor->ready = true;
//     // wait 200 us for as7341 to initialize
//     cyhal_system_delay_us(200);
//     // ATIME = 29
//     cyhal_i2c_master_mem_write(&(sensor->i2c_master), AS7341_I2C_ADDRESS, 0x81, 8, 29, 8, 80);
//     // ASTEP = 599
//     cyhal_i2c_master_mem_write(&(sensor->i2c_master), AS7341_I2C_ADDRESS, 0xCA, 8, 0b01010111, 8, 80);
//     cyhal_i2c_master_mem_write(&(sensor->i2c_master), AS7341_I2C_ADDRESS, 0xCB, 8, 0b00000010, 8, 80);
//     // power on and spectral measurement enable
//     cyhal_i2c_master_mem_write(&(sensor->i2c_master), AS7341_I2C_ADDRESS, 0x80, 8, pon_data, 8, 80);
// }