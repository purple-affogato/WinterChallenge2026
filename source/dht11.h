#ifndef DHT11_H

#define DHT11_H

#include "cyhal.h"

typedef struct dht11 DHT11;

struct dht11 {
    cyhal_gpio_t pin;
    uint8_t data[5];
    bool ready;
};

enum DHT11_RESULT {
    SUCCESS,
    TIMEOUT,
    CHECKSUM_ERROR
};

void initialize_dht11(DHT11 *sensor);
enum DHT11_RESULT read_dht11(DHT11 *sensor);
void print_dht11_data(DHT11 *sensor);

#endif // DHT11_H