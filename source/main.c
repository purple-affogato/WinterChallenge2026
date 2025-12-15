#include "cyhal.h"
#include "cybsp.h"
#include "cy_pdl.h"

#include "FreeRTOS.h"
#include "task.h"

#include "wifi_config.h"
#include "cy_wcm.h"

#include "cy_retarget_io.h"


void print_ipv4_address(cy_wcm_ip_address_t *addr) {
    printf("%d.%d.%d.%d\r\n",(int)addr->ip.v4>>0&0xFF,(int)addr->ip.v4>>8&0xFF,(int)addr->ip.v4>>16&0xFF,(int)addr->ip.v4>>24&0xFF);
}

void wifi_conn() {
    cy_rslt_t result;
    cy_wcm_config_t wcm_config = {.interface = CY_WCM_INTERFACE_TYPE_STA};
    cy_wcm_connect_params_t connect_params;
    cy_wcm_ip_address_t ip_addr;
    cy_wcm_mac_t mac_addr;
    cy_wcm_ip_address_t netmask;
    cy_wcm_ip_address_t gateway;

    result = cy_wcm_init(&wcm_config);

    // set up connection parameters
    memset(&connect_params, 0, sizeof(cy_wcm_connect_params_t));
    memcpy(connect_params.ap_credentials.SSID, WIFI_SSID, sizeof(WIFI_SSID));
    memcpy(connect_params.ap_credentials.password, WIFI_PASSWORD, sizeof(WIFI_PASSWORD));
    connect_params.ap_credentials.security = WIFI_SECURITY;

    for (uint8_t i=0;i<WIFI_RECONN_RETRIES;i++) {
        printf("\rAttempting to connect to wifi!\r\n");
        result = cy_wcm_connect_ap(&connect_params, &ip_addr);
        if (result == CY_RSLT_SUCCESS) {
            printf("Successfully connected!\r\n");

            printf("IP Address: ");
            print_ipv4_address(&ip_addr);

            cy_wcm_get_mac_addr(wcm_config.interface, &mac_addr);
            printf("MAC address: ");
            for(uint8_t j=0;j<CY_WCM_MAC_ADDR_LEN;j++) {
                uint8_t val = mac_addr[j];
                printf("%02X:",val);
            }
            printf("\r\n");

            cy_wcm_get_ip_netmask(wcm_config.interface, &netmask);
            printf("Netmask: ");
            print_ipv4_address(&netmask);

            cy_wcm_get_gateway_ip_address(wcm_config.interface, &gateway);
            printf("Gateway Address: ");
            print_ipv4_address(&gateway);
            break;
        }
    }

    while(1) {
        if (result == CY_RSLT_SUCCESS) {
            cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_ON);
            vTaskDelete(NULL);
        } else {
            cyhal_gpio_toggle(CYBSP_USER_LED);
        }
        vTaskDelay(100);
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
    vTaskStartScheduler();
}

/* [] END OF FILE */
