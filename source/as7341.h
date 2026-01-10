#ifndef AS7341_H
#define AS7341_H

#include "cyhal.h"

#define AS7341_I2C_ADDRESS (0x39)

struct as7341 {
    cyhal_gpio_t sda_pin;
    cyhal_gpio_t scl_pin;
    cyhal_i2c_t i2c_master;
    cyhal_gpio_t int_pin;
    cyhal_gpio_t gpio_pin;
    bool ready;
};

typedef struct as7341 AS7341;

void initialize_as7341(AS7341* sensor);

#endif // AS7341_H