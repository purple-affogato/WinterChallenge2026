#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "wifi_config.h"
#include "cy_wcm.h"
#include "cy_http_client_api.h"

#include "dht11.h"
#include "http_config.h"

bool connected = false;
bool wifi_work = false;
TaskHandle_t xWifiTaskHandle = NULL;
TaskHandle_t xHttpTaskHandle = NULL;

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

    cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);
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
            // print out all useful info
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
            wifi_work = true;
            xTaskNotifyGive(xHttpTaskHandle);
            vTaskDelay(100);
            vTaskDelete(NULL);
        } else {
            cyhal_gpio_toggle(CYBSP_USER_LED);
        }
        vTaskDelay(100);
    }
    
}

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

void disconnect_callback() {
    printf("disconnected from http server\r\n");
    connected = false;
}

void send_data_over_http() {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // wait for wifi connection
    if (!wifi_work) {
        printf("fuck!\r\n");
        vTaskDelete(NULL);
    }
    DHT11 d;
    d.pin = P13_1;
    d.ready = false;
    initialize_dht11(&d);
    enum DHT11_RESULT res = read_dht11(&d);
    if (res == TIMEOUT) {
        printf("DHT11 timed out.\r\n");
    } else if (res == CHECKSUM_ERROR) {
        printf("Checksum error.\r\n");
    } else {
        print_dht11_data(&d);
        cy_rslt_t cy_result;
        cy_awsport_server_info_t server_info;
        cy_awsport_ssl_credentials_t credentials;
        cy_http_client_t handle;

        (void) memset(&credentials, 0, sizeof(credentials));
	    (void) memset(&server_info, 0, sizeof(server_info));

        server_info.host_name = HOST_NAME;
        server_info.port = PORT_NUMBER;
        
        credentials.client_cert = (const char *) &SSL_CLIENTCERT_PEM;
        credentials.client_cert_size = sizeof( SSL_CLIENTCERT_PEM );
        credentials.private_key = (const char *) &SSL_CLIENTKEY_PEM;
        credentials.private_key_size = sizeof( SSL_CLIENTKEY_PEM );
        credentials.root_ca = (const char *) &SSL_ROOTCA_PEM;
        credentials.root_ca_size = sizeof( SSL_ROOTCA_PEM );

        cy_http_disconnect_callback_t dis_callback = (void*)disconnect_callback;
        
        cy_result = cy_http_client_init();
        if (cy_result != CY_RSLT_SUCCESS) {
            printf("couldn't initialize http client\r\n");
            vTaskDelete(NULL);
        }
        cy_result = cy_http_client_create(&credentials, &server_info, dis_callback, NULL, &handle);
        if (cy_result != CY_RSLT_SUCCESS) {
            printf("failed to create http client\r\n");
            vTaskDelete(NULL);
        }
        cy_result = cy_http_client_connect(handle, HTTP_TIMEOUT, HTTP_TIMEOUT);
        if (cy_result != CY_RSLT_SUCCESS) {
            printf("failed to connect http client\r\n");
            vTaskDelete(NULL);
        }
        connected = true;
        // creating request
        cy_http_client_request_header_t req;
        uint8_t userBuffer[2048];
        req.buffer = userBuffer;
        req.buffer_len = 2048;
        req.headers_len = 0;
        req.method = CY_HTTP_CLIENT_METHOD_POST;
        req.range_start = -1;
        req.range_end = -1;
        req.resource_path = RESOURCE;
        // headers
        cy_http_client_header_t header[1];
        header[0].field = "Content-Type";
        header[0].field_len = strlen("Content-Type");
        header[0].value = "application/json";
        header[0].value_len = strlen("application/json");
        cy_result = cy_http_client_write_header(handle, &req, header, 1);
        if (cy_result != CY_RSLT_SUCCESS) {
            printf("failed to write request header\r\n");
            return;
        }
        cy_http_client_response_t response;
        if (!connected) {
            printf("disconnected\r\n");
            return;
        }
        char reqBody[100];
        sprintf(reqBody, "{\"temp_integral\":%d,\"temp_decimal\":%d,\"rh_integral\":%d,\"rh_decimal\":%d}", d.data[2], d.data[3], d.data[0], d.data[1]);
        cy_result = cy_http_client_send(handle, &req, (uint8_t*)reqBody, strlen(reqBody), &response);
        if (cy_result != CY_RSLT_SUCCESS) {
            printf("failed to send request\r\n");
            return;
        }
        printf("Response status code %d\r\n", response.status_code);
    }
    while(1){
		vTaskDelay(1);
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

    xTaskCreate(wifi_conn, "wifi connection task", 1024, NULL, 1u, &xWifiTaskHandle);
    // xTaskCreate(test_dht11, "test dht11 task", 1024, NULL, 2, NULL);
    xTaskCreate(send_data_over_http, "testing http", 2048, NULL, 3u, &xHttpTaskHandle);
    vTaskStartScheduler();

    CY_ASSERT(0); // never reaches here hopefully
}

/* [] END OF FILE */
