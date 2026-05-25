#include <stdio.h>
#include <stdbool.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <esp_twai.h>
#include <esp_twai_onchip.h>

#include <esp_log.h>
#include <driver/gpio.h>

#define SPEED_QUERY             { 0x02, 0x01, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define ENGINELOAD_QUERY        { 0x02, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00 }

#define RED_LED GPIO_NUM_4
#define BLUE_LED GPIO_NUM_5
#define GREEN_LED GPIO_NUM_7

uint8_t twai_speed_query[8] = SPEED_QUERY; // Speed query
uint8_t twai_engineLoad_query[8] = ENGINELOAD_QUERY; // Engine load query

static const char *TAG_TWAI = "TWAI";

twai_node_handle_t node_hdl = NULL;

QueueHandle_t queue_twai;

typedef struct {
	uint32_t id;
	uint8_t data[8];
} rx_queue_msg_t;

twai_onchip_node_config_t node_config = {
	.io_cfg.tx = 19,
	.io_cfg.rx = 18,
	.bit_timing.bitrate = 500000,
	.tx_queue_depth = 5,
};

// 0x7E* filter
twai_mask_filter_config_t mask_cfg = {
	.id = 0x000,
	.mask = 0x000,
	.is_ext = false, 
}; 

twai_frame_t speed_query = {
	.header.id = 0x7DF, 
	.header.ide = false,
	.buffer = twai_speed_query,
    .buffer_len = sizeof(twai_speed_query), 
};	

void config_gpio(void) {
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask =
              (1ULL << RED_LED) |
              (1ULL << BLUE_LED) |
              (1ULL << GREEN_LED)
    };

    gpio_config(&io_conf);
}

void pulse_led(gpio_num_t ledNr) {
    gpio_set_level(ledNr, 1);

    vTaskDelay(pdMS_TO_TICKS(20));

    gpio_set_level(ledNr, 0);

    vTaskDelay(pdMS_TO_TICKS(20));
}

static bool twai_rx_cb(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx) {
	uint8_t recv_buff[8];
	rx_queue_msg_t msg;

	twai_frame_t rx_frame = {
		.buffer = recv_buff,
		.buffer_len = sizeof(recv_buff), 
	};

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	if (twai_node_receive_from_isr(handle, &rx_frame) == ESP_OK) {
        // ESP_LOGI(TAG_TWAI, ">> INFO: RX hit!");
		msg.id = rx_frame.header.id;
		memcpy(msg.data, recv_buff, rx_frame.buffer_len);
		xQueueOverwriteFromISR(queue_twai, &msg, &xHigherPriorityTaskWoken);		
	}

	return (xHigherPriorityTaskWoken == pdTRUE); 
}

twai_event_callbacks_t user_cbs = {
	.on_rx_done = twai_rx_cb,
};

void vTask_print_received_data(void *pvParameters) {
    rx_queue_msg_t msg;

    for (;;) {
        if (xQueueReceive(queue_twai, &msg, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG_TWAI, ">> Info: Received TWAI message with ID: 0x%X, Data: %02X %02X %02X %02X %02X %02X %02X %02X", 
                msg.id, msg.data[0], msg.data[1], msg.data[2], msg.data[3], 
                msg.data[4], msg.data[5], msg.data[6], msg.data[7]);
            
            pulse_led(BLUE_LED);
        }
    }
}

void vTask_send_query(void *pvParameters) {

    while (true) {
        
        esp_err_t err = twai_node_transmit(node_hdl, &speed_query, 0); 
        vTaskDelay(pdMS_TO_TICKS(400));
        ESP_LOGI(TAG_TWAI, ">> Info: Speed query has been sent. Return value: %s", esp_err_to_name(err));
        pulse_led(RED_LED);
        
    }

} 
    
void app_main(void){

    queue_twai = xQueueCreate(1, sizeof(rx_queue_msg_t));
    ESP_LOGI(TAG_TWAI, ">> Info: TWAI queue created successfully!");

    config_gpio();

    vTaskDelay(pdMS_TO_TICKS(500)); 

    ESP_ERROR_CHECK(twai_new_node_onchip(&node_config, &node_hdl));
	ESP_ERROR_CHECK(twai_node_register_event_callbacks(node_hdl, &user_cbs, NULL)); 
	ESP_ERROR_CHECK(twai_node_config_mask_filter(node_hdl, 0, &mask_cfg)); 
	ESP_ERROR_CHECK(twai_node_enable(node_hdl));

    ESP_LOGI(TAG_TWAI, ">> Info: TWAI node initialized and enabled successfully!");
    vTaskDelay(pdMS_TO_TICKS(500));

    xTaskCreate(vTask_print_received_data, "Print Received Data", 4096, NULL, 1, NULL);
    xTaskCreate(vTask_send_query, "TWAI Send Query", 2048, NULL, 1, NULL);
}