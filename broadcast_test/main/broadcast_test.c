#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <esp_random.h>
#include <driver/gpio.h>
#include <esp_rom_sys.h>

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

#define SPEED_TAG	    0x00
#define BROADCAST_ADDR   { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }

static const char *TAG_MAIN = "MAIN";
static const char *TAG_ESP_NOW_RECEIVE = "RECEIVE";

typedef struct {
	uint8_t tag;
	uint8_t speed_data;
	uint8_t engLoad_data;
} esp_now_data_packet_t; 

typedef struct { 
	uint8_t source_addr[ESP_NOW_ETH_ALEN];
	uint8_t destination_addr[ESP_NOW_ETH_ALEN];
	uint8_t tag;
	uint8_t speed_data;
	uint8_t engLoad_data; 
	int len;
} esp_now_data_packet_buff_t;


uint8_t random_uint8(void)
{
    return (uint8_t)(esp_random() & 0xFF);
}

void esp_now_recv_cb(const esp_now_recv_info_t *esp_now_info, const uint8_t *data, int len) {
			
	if (!esp_now_info || !data || len <= 0) return; 

    if (len != sizeof(esp_now_data_packet_t)) {
        ESP_LOGW(TAG_ESP_NOW_RECEIVE, "Invalid packet size: %d byte expected %d bytes. Packet dropped.",
                 len, sizeof(esp_now_data_packet_t));
        return;
    }

    const esp_now_data_packet_t *data_pkt = (const esp_now_data_packet_t *)data; // cast to structured packet for easier access to fields
    esp_now_data_packet_buff_t pkt;

	memcpy(pkt.source_addr, esp_now_info->src_addr, ESP_NOW_ETH_ALEN); 
	memcpy(pkt.destination_addr, esp_now_info->des_addr, ESP_NOW_ETH_ALEN); 
	pkt.tag = data_pkt->tag;
	pkt.len = len;
	pkt.speed_data = data_pkt->speed_data;
    pkt.engLoad_data = data_pkt->engLoad_data;

} 

esp_err_t esp_now_start(void) {

	esp_err_t ret = nvs_flash_init();
    vTaskDelay(pdMS_TO_TICKS(20));  
	ESP_ERROR_CHECK(ret); 								  
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); 				 
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
    vTaskDelay(pdMS_TO_TICKS(20));
	ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));
	ESP_ERROR_CHECK(esp_now_init());
    vTaskDelay(pdMS_TO_TICKS(20)); 

        ret = esp_now_register_recv_cb(esp_now_recv_cb);

	if (ret != ESP_OK) {	
        return ret;
	} else {
        return ESP_OK;
	}
}


uint8_t map_engLoad(uint8_t engLoad, uint8_t engLoadMapped) {
    engLoadMapped = (engLoad * 100) / 255; 
    return engLoadMapped;
}


void app_main(void)
{
    (void)esp_now_start();
    vTaskDelay(pdMS_TO_TICKS(100));  // Short delay to ensure ESP-NOW is initialized before proceeding with the rest of the setup
    uint8_t broadcast_addr[ESP_NOW_ETH_ALEN] = BROADCAST_ADDR;

    esp_now_peer_info_t peer = {0};
    memcpy(peer.peer_addr, broadcast_addr, ESP_NOW_ETH_ALEN);
    peer.ifidx = WIFI_IF_STA;
    peer.channel = 1;
    peer.encrypt = false;

    esp_err_t err = esp_now_add_peer(&peer);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG_MAIN, "ERROR: Failed to add broadcast peer: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG_MAIN, "INFO: Broadcast peer added successfully");
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    uint8_t random_data = 0;

    while (1) {

        for (int i = 0; i < 255; i++) {
            esp_now_data_packet_t pkt;
            pkt.tag = random_data;
            pkt.speed_data = random_data;
            pkt.engLoad_data = map_engLoad(random_data, pkt.engLoad_data);
            esp_now_send(broadcast_addr, (uint8_t *)&pkt, sizeof(pkt));
            ESP_LOGI(TAG_MAIN, "Broadcasted speed and Engine Load: %d %d", pkt.speed_data, pkt.engLoad_data);
            vTaskDelay(pdMS_TO_TICKS(150));  // Send every 800 ms
            random_data++;
        }      

        for (int i = 255; i > 0; i--) {
            esp_now_data_packet_t pkt;
            pkt.tag = random_data;
            pkt.speed_data = random_data;
            pkt.engLoad_data = map_engLoad(random_data, pkt.engLoad_data);
            esp_now_send(broadcast_addr, (uint8_t *)&pkt, sizeof(pkt));
            ESP_LOGI(TAG_MAIN, "Broadcasted speed and Engine Load: %d %d", pkt.speed_data, pkt.engLoad_data);
            vTaskDelay(pdMS_TO_TICKS(150));  // Send every 800 ms
            random_data--;
        }
    }
}