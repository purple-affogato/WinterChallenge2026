#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

#include "FreeRTOS.h"
#include "task.h"

#include "wifi_config.h"
#include "cy_wcm.h"

#include "dht11.h"

void test_dht11() {
    DHT11 d;
    d.pin = P13_1;
    d.ready = false;
    initialize_dht11(&d);
    for (;;) {
        enum DHT11_RESULT res = read_dht11(&d);
        if (res == TIMEOUT) {
            printf("DHT11 timed out.\r\n");
            
        } else if (res == CHECKSUM_ERROR) {
            printf("Checksum error.\r\n");
            
        } else {
            print_dht11_data(&d);
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    result = cy_retarget_io_init_fc(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX,
            CYBSP_DEBUG_UART_CTS,CYBSP_DEBUG_UART_RTS,CY_RETARGET_IO_BAUDRATE);
    CY_ASSERT(result == CY_RSLT_SUCCESS);

    printf("\033[2J\033[H"); // clears output screen

    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);

    xTaskCreate(wifi_conn, "wifi connection task", 1024, NULL, 1, NULL);
    xTaskCreate(test_dht11, "test dht11 task", 1024, NULL, 2, NULL);
    vTaskStartScheduler();
}

/* [] END OF FILE */
