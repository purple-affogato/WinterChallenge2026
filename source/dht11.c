#include "dht11.h"
#include "cyhal.h"
#include "cy_retarget_io.h"

const cyhal_timer_cfg_t timer_cfg = {
    .is_continuous = false,
    .direction = CYHAL_TIMER_DIR_UP,
    .is_compare = false,
    .period = 200,
    .compare_value = 0,
    .value = 0
};

void initialize_dht11(DHT11 *sensor) {
    CY_ASSERT(sensor != NULL);
    cy_rslt_t result;
    result = cyhal_gpio_init(sensor->pin, CYHAL_GPIO_DIR_BIDIRECTIONAL, CYHAL_GPIO_DRIVE_PULLUP, true);
    CY_ASSERT(result == CY_RSLT_SUCCESS);
    sensor->ready = true;
}

enum DHT11_RESULT read_dht11(DHT11 *sensor) {
    CY_ASSERT(sensor->ready == true);
    cy_rslt_t result;
    // uint8_t stage = 0; // for debugging purposes
    // reset data to 0
    for(uint8_t i=0;i<5;i++) {
        sensor->data[i] = 0;
    }
    cyhal_timer_t timer; // initialize timer
    result = cyhal_timer_init(&timer, NC, NULL);
    CY_ASSERT(result == CY_RSLT_SUCCESS);
    cyhal_timer_configure(&timer, &timer_cfg);
    cyhal_timer_set_frequency(&timer, 1000000); // counts every 1us
    // send a start signal
    cyhal_gpio_write(sensor->pin, false);
    cyhal_system_delay_ms(18);
    cyhal_gpio_write(sensor->pin, true);
    // wait for DHT11 to pull down voltage
    cyhal_timer_start(&timer);
    while(cyhal_gpio_read(sensor->pin) == true) {
        if (cyhal_timer_read(&timer) > 70) { // waiting 20-40us for falling edge
            // printf("stage %d\r\n", stage);
            return TIMEOUT;
        }
    }
    // stage++;
    // process for start signal
    cyhal_timer_reset(&timer);
    while(cyhal_gpio_read(sensor->pin) == false) { // dht holds low voltage for ~80us
        if (cyhal_timer_read(&timer) > 90) {
            // printf("stage %d\r\n", stage);
            return TIMEOUT;
        }
    }
    // stage++;
    cyhal_timer_reset(&timer);
    while(cyhal_gpio_read(sensor->pin) == true) { // dht holds high voltage for ~80us
        if (cyhal_timer_read(&timer) > 90) {
            // printf("stage %d\r\n", stage);
            return TIMEOUT;
        }
    }
    // stage++;
    // start reading data
    // RH integral data, RH decimal data, T integral, T decimal
    for (uint8_t i=0;i<5;i++) {
        for (uint8_t j=0;j<8;j++) {
            cyhal_timer_reset(&timer);
            while(cyhal_gpio_read(sensor->pin) == false) { // wait 50us on low
                if (cyhal_timer_read(&timer) > 60) {
                    // printf("stage %d\r\n", stage);
                    return TIMEOUT;
                }
            }
            cyhal_timer_reset(&timer);
            while(cyhal_gpio_read(sensor->pin) == true) { // 70us = 1, 26-28us = 0
                if (cyhal_timer_read(&timer) > 80) {
                    // printf("stage %d\r\n", stage);
                    return TIMEOUT;
                }
            }
            sensor->data[i] <<= 1;
            if (cyhal_timer_read(&timer) > 60) { // check if a '1' was read
                sensor->data[i] = sensor->data[i] | 1;
            }
        }
    }
    // not checking for ending signal, but putting a delay between each communication process mitigates that
    // for reference, each process takes 4ms
    cyhal_timer_free(&timer);
    if (sensor->data[0] + sensor->data[1] + sensor->data[2] + sensor->data[3] == sensor->data[4])
        return SUCCESS;
    return CHECKSUM_ERROR;
}

void print_dht11_data(DHT11 *sensor) {
    printf("%d.%d C and %d.%d RH\r\n", sensor->data[2], sensor->data[3], sensor->data[0], sensor->data[1]);
}