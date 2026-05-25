#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>

#include <esp_timer.h>
#include <esp_twai.h>
#include <esp_twai_onchip.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <esp_sleep.h>

#define BUS_STATUS_FLAG             (1 << 0)

#define TASK_SEND_QUERY_FLAG        (1 << 0) 
#define TASK_RECEIVE_TWAI_FLAG      (1 << 1)
#define TASK_ESP_NOW_SEND_DATA_FLAG (1 << 2)
#define TASK_IDLE_MODE_FLAG 		(1 << 3)  

#define BROADCAST_MAC           { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
#define BROADCAST_PASS          31
#define SPEED_QUERY             { 0x02, 0x01, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define ENGINELOAD_QUERY        { 0x02, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00 }

#define TWAI_SPEED_TAG          0x0D
#define TWAI_ENGINELOAD_TAG     0x04

#define TIMEOUT_IN_MS 10000
#define COOLDOWN_TIME_MS 3000
#define TWAI_CHECK_TIME_MS 5000

#define QUERY_INTERVAL 400 

typedef struct {
	uint32_t id;
	uint8_t data[8];
} rx_queue_msg_t;

typedef struct {
    uint8_t speed;
    uint8_t engineLoad;
} esp_now_data_buffer_t;

typedef struct {
	uint8_t tag;
	uint8_t speed_data;
	uint8_t engLoad_data;
} esp_now_data_packet_t; 

QueueHandle_t queue_twai;
QueueHandle_t queue_esp_now;

EventGroupHandle_t STATUS_REG   = NULL; 		
EventGroupHandle_t TASK_REG     = NULL;

static bool esp_now_initialized = false;
static bool wifi_initialized 	= false; 

static const char *TAG_TWAI 			= "TWAI";
static const char *TAG_ESP_NOW 			= "ESP-NOW";
static const char *TAG_RECEIVE 			= "RECEIVE";
static const char *TAG_IDLE_MODE 		= "IDLE MODE";
static const char *TAG_MAIN 			= "MAIN";

twai_node_handle_t node_hdl = NULL;

twai_onchip_node_config_t node_config = {
	.io_cfg.tx = 20,
	.io_cfg.rx = 21,
	.bit_timing.bitrate = 500000,
	.tx_queue_depth = 5,
};

twai_mask_filter_config_t mask_cfg = {
	.id = 0x7E8,
	.mask = 0x7E0,
	.is_ext = false, 
}; 

uint8_t twai_speed_query[8] = SPEED_QUERY; // Speed query
uint8_t twai_engineLoad_query[8] = ENGINELOAD_QUERY; // Engine load query
									      
twai_frame_t speed_query = {
	.header.id = 0x7E8, 
	.header.ide = false,
	.buffer = twai_speed_query,
    .buffer_len = sizeof(twai_speed_query), 
};	

twai_frame_t engine_load_query = {
	.header.id = 0x7E8, 
	.header.ide = false, 
	.buffer = twai_engineLoad_query,
	.buffer_len = sizeof(twai_engineLoad_query),
};

// YAPILACAKLAR: 
// burada gelen sinyalin validatelenmesi lazim
static bool twai_rx_cb(twai_node_handle_t handle, const twai_rx_done_event_data_t *edata, void *user_ctx) {
	uint8_t recv_buff[8];
	rx_queue_msg_t msg;

	twai_frame_t rx_frame = {
		.buffer = recv_buff,
		.buffer_len = sizeof(recv_buff), 
	};

	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	if (twai_node_receive_from_isr(handle, &rx_frame) == ESP_OK) {
		msg.id = rx_frame.header.id;
		memcpy(msg.data, recv_buff, rx_frame.buffer_len);
		xQueueOverwriteFromISR(queue_twai, &msg, &xHigherPriorityTaskWoken);		
	}

	return (xHigherPriorityTaskWoken == pdTRUE); 
}

twai_event_callbacks_t user_cbs = {
	.on_rx_done = twai_rx_cb,
};

void esp_now_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int len) {
// data gelmeyecek
} 

esp_err_t esp_now_start(void) {

	esp_err_t ret = nvs_flash_init();  
	ESP_ERROR_CHECK(ret); 								  
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); 				 
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
	

	ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));
	ESP_ERROR_CHECK(esp_now_init()); 
    

	ret = esp_now_register_recv_cb(esp_now_recv_cb);

	if (ret == ESP_OK) {
		ESP_LOGI(TAG_ESP_NOW, "INFO: ESP-NOW started succesfully!");
		wifi_initialized = true; 
		esp_now_initialized = true;
		return ESP_OK;		
	} else {
		ESP_LOGE(TAG_ESP_NOW, "ERROR: Failed to register receive callback: %s", esp_err_to_name(ret));
        return ret;
	}
}



esp_err_t esp_now_stop(void) {
	
	esp_err_t err = ESP_OK;
	
	if (esp_now_initialized) {
		(void)esp_now_deinit();
		vTaskDelay(pdMS_TO_TICKS(20));
		esp_now_initialized = false;
	}
		
	if (wifi_initialized) {
		(void)esp_wifi_stop();
		vTaskDelay(pdMS_TO_TICKS(20));
		err = esp_wifi_deinit();
		vTaskDelay(pdMS_TO_TICKS(20));
		wifi_initialized = false;
	}

	esp_err_t nvs_err = nvs_flash_deinit();
    vTaskDelay(pdMS_TO_TICKS(20));

	if (err != ESP_OK || nvs_err != ESP_OK) { 
        ESP_LOGE(TAG_ESP_NOW, "ERROR: Failed to deinitialize ESP-NOW: %s", esp_err_to_name(err));
        return err != ESP_OK ? err : nvs_err;   
	}
	
	ESP_LOGI(TAG_ESP_NOW, "INFO: ESP-NOW stopped successfully");	
	return ESP_OK;
    
} 



// receive data, if valid decode data and prepare the espnow package and send it to sender task. If no data comes enable idle mode. 
void vTask_receive_twai(void *pvParameters) { 

    rx_queue_msg_t msg;
    uint8_t counter = 0;
    esp_now_data_buffer_t pkt_buffer = {0};
    int64_t last_valid_packet_time = esp_timer_get_time();  

    for (;;) {

        xEventGroupWaitBits(TASK_REG, TASK_RECEIVE_TWAI_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);
        last_valid_packet_time = esp_timer_get_time();

        while (xEventGroupGetBits(TASK_REG) & TASK_RECEIVE_TWAI_FLAG) {

            if (xQueueReceive(queue_twai, &msg, pdMS_TO_TICKS(10)) == pdPASS) {
                
                if (msg.data[2] != TWAI_SPEED_TAG && msg.data[2] != TWAI_ENGINELOAD_TAG) {
                    continue;
                }
                
                last_valid_packet_time = esp_timer_get_time();

                if (msg.data[2] == TWAI_SPEED_TAG) {
                    pkt_buffer.speed = msg.data[3];
                    counter++;
                } else if (msg.data[2] == TWAI_ENGINELOAD_TAG) {
                    pkt_buffer.engineLoad = msg.data[3];
                    counter++;
                } 

                if (counter >= 2) {
                    xQueueOverwrite(queue_esp_now, &pkt_buffer);
                    counter = 0;
					xEventGroupSetBits(STATUS_REG, BUS_STATUS_FLAG);
                }
            }

            int64_t current_time = esp_timer_get_time();
            if ((current_time - last_valid_packet_time) / 1000 > TIMEOUT_IN_MS) {

                ESP_LOGW(TAG_RECEIVE, ">> Warning: No response from ECU for %d ms, going idle", TIMEOUT_IN_MS);

                xEventGroupClearBits(TASK_REG, TASK_SEND_QUERY_FLAG);
                xEventGroupClearBits(TASK_REG, TASK_ESP_NOW_SEND_DATA_FLAG);
                xEventGroupClearBits(TASK_REG, TASK_RECEIVE_TWAI_FLAG);

				// esp_nowu durdur
                esp_err_t err = esp_now_stop(); 
                if (err == ESP_OK) {
                    ESP_LOGI(TAG_ESP_NOW, ">> Info: ESP-NOW is now stopped");
                } else {
					ESP_LOGE(TAG_ESP_NOW, ">> Error: Failed to stop ESP-NOW: %s", esp_err_to_name(err));
				}
 				
				// Idle'a geç
				xEventGroupSetBits(TASK_REG, TASK_IDLE_MODE_FLAG);

				// Gecmeden once twaiyi kapat. Counteri ve bufferi sifirla.
                esp_err_t twai_disable_err = twai_node_disable(node_hdl);
				if (twai_disable_err == ESP_OK) {
					ESP_LOGI(TAG_TWAI, ">> INFO: TWAI node successfully disabled");
				} else {
					ESP_LOGE(TAG_TWAI, ">> INFO: TWAI node could not be disabled. Error: %s", esp_err_to_name(twai_disable_err));
				}

                counter = 0;
                memset(&pkt_buffer, 0, sizeof(pkt_buffer));

            }

            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

// degismeli olarak speed ve engine load querysi atiyor. (EN SON HATA BURADAYDI. Node bus is off hatasi )
void vTask_send_query(void *pvParameters) {
	bool query_type = false;

	for (;;) {

		xEventGroupWaitBits(TASK_REG, TASK_SEND_QUERY_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);  
		
		while (xEventGroupGetBits(TASK_REG) & TASK_SEND_QUERY_FLAG) {
			
			if (query_type == false) {
				esp_err_t err = twai_node_transmit(node_hdl, &speed_query, 0); 
				query_type = !query_type;
				vTaskDelay(pdMS_TO_TICKS(QUERY_INTERVAL));
				ESP_LOGI(TAG_TWAI, ">> Info: Speed query has been sent. Return value is: %s", esp_err_to_name(err));
			} else {
				esp_err_t err = twai_node_transmit(node_hdl, &engine_load_query, 0);
				query_type = !query_type;
				vTaskDelay(pdMS_TO_TICKS(QUERY_INTERVAL));
				ESP_LOGI(TAG_TWAI, ">> Info: Engine load query has been sent. Return value is: %s", esp_err_to_name(err)); 
			}	
		
		}

		vTaskDelay(pdMS_TO_TICKS(QUERY_INTERVAL));
	}
} 

// pktyi receivetaskden alir ve espnow ile broadcast adresine gonderir
void vTask_esp_now_send_data(void *args) {
	esp_now_data_packet_t pkt; 
	esp_now_data_buffer_t pkt_buffer;

	uint8_t broadcast_addr[ESP_NOW_ETH_ALEN] = BROADCAST_MAC;
	esp_now_peer_info_t peer = {0};
	
	memcpy(peer.peer_addr, broadcast_addr, ESP_NOW_ETH_ALEN);
	peer.ifidx = WIFI_IF_STA;
    peer.channel = 0;
	peer.encrypt = false;
	(void)esp_now_add_peer(&peer);	
		
	for (;;) {

		xEventGroupWaitBits(TASK_REG, TASK_ESP_NOW_SEND_DATA_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);

		while (xEventGroupGetBits(TASK_REG) & TASK_ESP_NOW_SEND_DATA_FLAG) {

			if (xQueueReceive(queue_esp_now, &pkt_buffer, portMAX_DELAY) == pdPASS) {

				pkt.tag = BROADCAST_PASS;
				pkt.speed_data = pkt_buffer.speed;
				pkt.engLoad_data = pkt_buffer.engineLoad;

				esp_err_t err = esp_now_send(peer.peer_addr, (uint8_t *)&pkt, sizeof(pkt)); 

				if (err == ESP_OK) {
					ESP_LOGI(TAG_ESP_NOW, ">> Info: ESP-NOW packet sent successfully! Speed: %d, Engine Load: %d", pkt.speed_data, pkt.engLoad_data);
				} else {
					ESP_LOGE(TAG_ESP_NOW, ">> Warning: ESP-NOW packet could not be sent. Error> %s", esp_err_to_name(err));
				}

			}
			
			vTaskDelay(pdMS_TO_TICKS(20));
		}

		vTaskDelay(pdMS_TO_TICKS(20));
	}
}


void vTask_idle_mode(void *pvParameters) {
    
    for (;;) {
		
        xEventGroupWaitBits(TASK_REG, TASK_IDLE_MODE_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);

        while (xEventGroupGetBits(TASK_REG) & TASK_IDLE_MODE_FLAG) {
            ESP_LOGW(TAG_IDLE_MODE, "INFO: Entering idle mode for %d Seconds!", COOLDOWN_TIME_MS / 1000);

			// esp_sleep_enable_timer_wakeup(COOLDOWN_TIME_MS * 1000);
			// esp_light_sleep_start();
			
			// dummy sleep
            long long start_time = esp_timer_get_time() / 1000;
            while ((esp_timer_get_time() / 1000) - start_time < COOLDOWN_TIME_MS) {  // COOLDOWN_TIME_MS in milliseconds
                 ESP_LOGI(TAG_IDLE_MODE, ">> Info: Idle mode active, waiting for wake-up signal. Time left: %lld milliseconds",
                    COOLDOWN_TIME_MS - ((esp_timer_get_time() / 1000) - start_time));
                vTaskDelay(pdMS_TO_TICKS(1000));  
            }
			
            ESP_LOGW(TAG_IDLE_MODE, "Warning: Woke-Up! Starting TWAI_send and TWAI_receive. Checking for signals");
			
			(void)twai_node_enable(node_hdl);				 
			vTaskDelay(pdMS_TO_TICKS(50));

            xEventGroupSetBits(TASK_REG, TASK_RECEIVE_TWAI_FLAG);
			xEventGroupSetBits(TASK_REG, TASK_SEND_QUERY_FLAG);

            // Wait for connection flag (TWAI_CHECK_TIME_MS).
			EventBits_t bits = xEventGroupWaitBits(STATUS_REG, BUS_STATUS_FLAG, pdTRUE, pdTRUE, pdMS_TO_TICKS(TWAI_CHECK_TIME_MS));       
            if (bits & BUS_STATUS_FLAG) {

                xEventGroupClearBits(TASK_REG, TASK_IDLE_MODE_FLAG);
				xEventGroupSetBits(TASK_REG, TASK_ESP_NOW_SEND_DATA_FLAG);

				ESP_LOGI(TAG_IDLE_MODE, ">> Warning: Valid signal detected, exiting idle mode."); 
                vTaskDelay(pdMS_TO_TICKS(100));  

                break;

            } else {

                xEventGroupClearBits(TASK_REG, TASK_SEND_QUERY_FLAG);
                xEventGroupClearBits(TASK_REG, TASK_ESP_NOW_SEND_DATA_FLAG);
                xEventGroupClearBits(TASK_REG, TASK_RECEIVE_TWAI_FLAG);

                (void)esp_now_stop();               
                (void)twai_node_disable(node_hdl); 

				ESP_LOGW(TAG_IDLE_MODE, "WARNING: No signal. Stopping ESP-NOW and Receive. Starting over!");       
            }
        }
    }
}


void app_main(void) {

	TASK_REG = xEventGroupCreate();
	STATUS_REG = xEventGroupCreate();

	queue_twai = xQueueCreate(1, sizeof(rx_queue_msg_t));
	queue_esp_now = xQueueCreate(1, sizeof(esp_now_data_buffer_t));
	
	ESP_ERROR_CHECK(twai_new_node_onchip(&node_config, &node_hdl));
	ESP_ERROR_CHECK(twai_node_register_event_callbacks(node_hdl, &user_cbs, NULL)); 
	ESP_ERROR_CHECK(twai_node_config_mask_filter(node_hdl, 0, &mask_cfg)); 
	ESP_ERROR_CHECK(twai_node_enable(node_hdl)); 

	xTaskCreate(vTask_receive_twai, "TWAI Receive", 4096, NULL, 3, NULL);

	vTaskDelay(pdMS_TO_TICKS(50));

	if (esp_now_start() != ESP_OK) {
		ESP_LOGE(TAG_MAIN, "Failed to start ESP-NOW");
	}

	vTaskDelay(pdMS_TO_TICKS(50));

	xTaskCreate(vTask_send_query, "TWAI Send Query", 2048, NULL, 1, NULL);
	xTaskCreate(vTask_esp_now_send_data, "ESPNOW Send Data", 4096, NULL, 3, NULL);
	xTaskCreate(vTask_idle_mode, "Idle Mode", 3072, NULL, 2, NULL);
	
	xEventGroupSetBits(TASK_REG, TASK_RECEIVE_TWAI_FLAG);
	xEventGroupSetBits(TASK_REG, TASK_SEND_QUERY_FLAG);
	xEventGroupSetBits(TASK_REG, TASK_ESP_NOW_SEND_DATA_FLAG);

}
