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
        cy_http_client_t handle;
        server_info.host_name = HOST_NAME;
        server_info.port = PORT_NUMBER;
        cy_http_disconnect_callback_t dis_callback = (void*)disconnect_callback;
        cy_result = cy_http_client_init();
        if (cy_result != CY_RSLT_SUCCESS) {
            printf("couldn't initialize http client\r\n");
            return;
        }
        cy_result = cy_http_client_create(NULL, &server_info, dis_callback, NULL, &handle);
        if (cy_result != CY_RSLT_SUCCESS) {
            printf("failed to create http client\r\n");
            return;
        }
        cy_result = cy_http_client_connect(handle, 5000, 5000);
        if (cy_result != CY_RSLT_SUCCESS) {
            printf("failed to connect http client\r\n");
            return;
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
        req.resource_path = "/receive-data-once";
        // headers
        uint32_t num_header = 1;
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
        cy_result = cy_http_client_send(handle, &req, reqBody, strlen(reqBody), &response);
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

    xTaskCreate(wifi_conn, "wifi connection task", 1024, NULL, 1, NULL);
    xTaskCreate(test_dht11, "test dht11 task", 1024, NULL, 2, NULL);
    vTaskStartScheduler();
}

/* [] END OF FILE */
