#include <hud_display.h>

static const char *TAG_ESP_NOW = "ESP-NOW";
static const char *TAG_ESP_NOW_RECEIVE = "RECEIVE";
static const char *TAG_IDLE_MODE = "IDLE MODE";
static const char *TAG_LED = "LED";
static const char *TAG_MAIN = "MAIN";

QueueHandle_t queue_esp_now_recv; 
QueueHandle_t queue_display;

EventGroupHandle_t STATUS_REG   = NULL; 		
EventGroupHandle_t TASK_REG     = NULL;

uint8_t global_peer_addr[ESP_NOW_ETH_ALEN] = {0};
static const uint8_t empty_addr[ESP_NOW_ETH_ALEN] = {0};

gpio_num_t pins[PIN_COUNT] = {
    P1,
    P4,
    P5,
    P6,
    P7,
    P10,
};

// in reality, led index starts at 3. (0, 1, 2 are not are not charlieplexed)
led_t led[33] = {
    {100, 100}, // dummy
    {100, 100}, // dummy
    {100, 100}, // dummy
    {P10, P1}, // LED3
    {P10, P4}, // LED4 etc...
    {P10, P6},
    {P10, P7},
    {P10, P5},
    {P5, P1},
    {P5, P4},
    {P5, P6},
    {P5, P7},
    {P5, P10},
    {P7, P1},
    {P7, P4},
    {P7, P6},
    {P7, P5},
    {P7, P10},
    {P6, P1},
    {P6, P4},
    {P6, P7},
    {P6, P5},
    {P6, P10},
    {P4, P1},
    {P4, P6},
    {P4, P7},
    {P4, P5},
    {P4, P10},
    {P1, P4},
    {P1, P6},
    {P1, P7},
    {P1, P5},
    {P1, P10},
};


const uint8_t leds_per_number[10] = {
    NR0_LED_COUNT,
    NR1_LED_COUNT,
    NR2_LED_COUNT,
    NR3_LED_COUNT,
    NR4_LED_COUNT,
    NR5_LED_COUNT,
    NR6_LED_COUNT,
    NR7_LED_COUNT,
    NR8_LED_COUNT,
    NR9_LED_COUNT,
};

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

	xQueueSend(queue_esp_now_recv, &pkt, portMAX_DELAY);
} 


void make_all_hiz(void) {
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
        .pin_bit_mask = 0
	};

	for (int i = 0; i < PIN_COUNT; i++) {
	io_conf.pin_bit_mask |= (1ULL << pins[i]);
	}

    gpio_config(&io_conf);
}


void parse_speed_data(uint8_t speed, uint8_t *hundreds, uint8_t *tens, uint8_t *ones) {
    *hundreds = speed / 100;   // hundreds place
    *tens = (speed / 10) % 10; // tens place
    *ones = speed % 10;        // ones place
}

// count - 1 olmasinin sebebi diger turlu 7. yani olmayan bir array degerinin gitmesi.
void map_engLoad(uint8_t engLoad, uint8_t *engLoadBarCount) {
    uint8_t mapped = ((engLoad * BAR_LED_COUNT) / 100);
    if (mapped > BAR_LED_COUNT - 1) mapped = BAR_LED_COUNT - 1;  
    *engLoadBarCount = mapped;
}

void displayNumber(led_t digits_numbers_leds[DIGIT_COUNT][NUMBER_COUNT][MAX_LED_PER_DIGIT], uint8_t digit, uint8_t number, uint8_t led_count) {
    for (int i = 0; i < led_count; i++) {
        pulse_led(digits_numbers_leds[digit][number][i]);
    }
}

// mapped_engine
void displayEngLoad(led_t engLoad_bar[BAR_LED_COUNT][BAR_LED_COUNT], uint8_t mapped_engLoad, uint8_t led_count) {
    for (int i = 0; i < led_count; i++) {
        pulse_led(engLoad_bar[mapped_engLoad][i]);
    }
}



void pulse_led(led_t led) {
    gpio_set_direction(led.high, GPIO_MODE_OUTPUT);
    gpio_set_direction(led.low, GPIO_MODE_OUTPUT);

    gpio_set_level(led.high, 1);
    gpio_set_level(led.low, 0);
    // vTaskDelay(pdMS_TO_TICKS(LED_ON_TIME_MS)); 
    esp_rom_delay_us(LED_ON_TIME);

    gpio_set_direction(led.high, GPIO_MODE_INPUT);
    gpio_set_direction(led.low, GPIO_MODE_INPUT);
    // vTaskDelay(pdMS_TO_TICKS(LED_OFF_TIME_MS)); 
    // esp_rom_delay_us(LED_OFF_TIME);
}


esp_err_t esp_now_start(void) {

	esp_err_t ret = nvs_flash_init();
    // ESP_ERROR_CHECK(esp_event_loop_create_default());  
	ESP_ERROR_CHECK(ret); 								  
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); 				 
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_start());
	ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));
	ESP_ERROR_CHECK(esp_now_init()); 
    

	ret = esp_now_register_recv_cb(esp_now_recv_cb);

	if (ret != ESP_OK) {
		ESP_LOGE(TAG_ESP_NOW, "ERROR: Failed to register receive callback: %s", esp_err_to_name(ret));
        return ret;
	} else {
		ESP_LOGI(TAG_ESP_NOW, "INFO: ESP-NOW started succesfully!");
        xEventGroupSetBits(STATUS_REG, STATUS_ESP_NOW_FLAG);
        return ESP_OK;
	}
}


esp_err_t esp_now_stop(void) {
	
	(void)esp_now_deinit();
	vTaskDelay(pdMS_TO_TICKS(20));
	(void)esp_wifi_stop();
	vTaskDelay(pdMS_TO_TICKS(20));
	esp_err_t err = esp_wifi_deinit();
	vTaskDelay(pdMS_TO_TICKS(20));
    esp_err_t nvs_err = nvs_flash_deinit();
    vTaskDelay(pdMS_TO_TICKS(20));

	if (err != ESP_OK || nvs_err != ESP_OK) { 
        ESP_LOGE(TAG_ESP_NOW, "ERROR: Failed to deinitialize ESP-NOW: %s", esp_err_to_name(err));
        return err != ESP_OK ? err : nvs_err;  // Return the first error encountered  
	} else {
        ESP_LOGI(TAG_ESP_NOW, "INFO: ESP-NOW stopped successfully");
        memset(global_peer_addr, 0, ESP_NOW_ETH_ALEN); // Reset registered peer address
	    xEventGroupClearBits(STATUS_REG, STATUS_ESP_NOW_FLAG);
        return ESP_OK;
    }
} 




// receives Packets, checks for validity, and sends to display queue. Also handles connection timeout and idle mode transition.
void vTask_esp_now_receive(void *args)
{
    uint8_t broadcast_addr[ESP_NOW_ETH_ALEN] = BROADCAST_ADDR;
    esp_now_data_packet_buff_t pkt;

    int64_t last_valid_packet_time = esp_timer_get_time();

    for (;;) {

        xEventGroupWaitBits( TASK_REG, TASK_ESP_NOW_RECEIVE_FLAG, pdFALSE, pdFALSE, portMAX_DELAY );
        last_valid_packet_time = esp_timer_get_time();

        while (xEventGroupGetBits(TASK_REG) & TASK_ESP_NOW_RECEIVE_FLAG) {

            int64_t current_time = esp_timer_get_time();

            if ((current_time - last_valid_packet_time) / 1000 > TIMEOTUT_CHECK_MS) {

                ESP_LOGW(TAG_ESP_NOW_RECEIVE, ">> Warning: No valid packets for %d seconds. Connection lost. Stopping ESP-NOW, Receive Task, Display Task and starting IDLE mode", TIMEOTUT_CHECK_MS / 1000);

                (void)esp_now_stop();
                vTaskDelay(pdMS_TO_TICKS(20));
                xEventGroupClearBits(STATUS_REG, STATUS_ESP_NOW_CONNECTION_FLAG);
                xEventGroupClearBits(TASK_REG, TASK_DISPLAY_FLAG);
                xEventGroupClearBits(TASK_REG, TASK_ESP_NOW_RECEIVE_FLAG);
                xEventGroupSetBits(TASK_REG, TASK_DUMMY_IDLE_MODE_FLAG);  
                
                last_valid_packet_time = current_time;
            }

            //  PACKET RECEIVE  
            if (xQueueReceive(queue_esp_now_recv, &pkt, pdMS_TO_TICKS(50)) == pdTRUE) {
                
                // Check if packet is broadcast. If not, drop.
                if (memcmp(broadcast_addr, pkt.destination_addr, ESP_NOW_ETH_ALEN) != 0) {
                    ESP_LOGW(TAG_ESP_NOW_RECEIVE, ">> Warning: Unicast packet dropped");
                    continue;
                }

                // If first valid packet, register peer address.
                if (memcmp(empty_addr, global_peer_addr, ESP_NOW_ETH_ALEN) == 0) {
                    memcpy(global_peer_addr, pkt.source_addr, ESP_NOW_ETH_ALEN);

                    ESP_LOGW(TAG_ESP_NOW,
                        ">> Registered peer: %02X:%02X:%02X:%02X:%02X:%02X",
                        global_peer_addr[0], global_peer_addr[1], global_peer_addr[2],
                        global_peer_addr[3], global_peer_addr[4], global_peer_addr[5]);
                }

                // Check if packet is from a known peer. If not, drop.
                if (memcmp(global_peer_addr, pkt.source_addr, ESP_NOW_ETH_ALEN) != 0) {
                    ESP_LOGW(TAG_ESP_NOW_RECEIVE, ">> Warning: Unknown address, packet dropped");
                    continue;
                }

                // valid packet
                last_valid_packet_time = esp_timer_get_time();
                xEventGroupSetBits(STATUS_REG, STATUS_ESP_NOW_CONNECTION_FLAG);
                xEventGroupSetBits(TASK_REG, TASK_DISPLAY_FLAG);
                vTaskDelay(pdMS_TO_TICKS(5));

                xQueueOverwrite(queue_display, &pkt);
            }
        }
    }
}


// receives data from receive task, parses and displays on LEDs.
void vTask_display(void *args) {

	esp_now_data_packet_buff_t pkt;
    uint8_t hundreds = 0, tens = 0, ones = 0;
    uint8_t engLoadBarCount = 0;
    bool have_value = false;
    led_t led_buffer[30]; // Declare the LED array here to be used in the display functions

    led_t engLoad_bar[BAR_LED_COUNT][BAR_LED_COUNT] = {
        {led[12]},
        {led[12], led[7]},
        {led[12], led[7], led[6]},
        {led[12], led[7], led[6], led[5]},
        {led[12], led[7], led[6], led[5], led[9]},
        {led[12], led[7], led[6], led[5], led[9], led[4]},
        {led[12], led[7], led[6], led[5], led[9], led[4], led[3]}
    };

    led_t battery[3] = {
        led[18], led[23], led[28]
    };

	led_t digits_numbers_leds[DIGIT_COUNT][NUMBER_COUNT][MAX_LED_PER_DIGIT] = {
        
        {// digit 0
         {led[26], led[21], led[16], led[22], led[17], led[26]},
         {led[21], led[16]},
         {led[26], led[21], led[27], led[22], led[17]},
         {led[26], led[12], led[27], led[16], led[17]},
         {led[21], led[27], led[16], led[21]},
         {led[26], led[27], led[16], led[17], led[26]},
         {led[26], led[27], led[22], led[16], led[17], led[26]},
         {led[26], led[21], led[16]},
         {led[26], led[21], led[27], led[16], led[22], led[17], led[26]},
         {led[26], led[21], led[27], led[16], led[17], led[26]}
        },
        {// digit 1
         {led[11], led[15], led[14], led[25], led[31], led[32]},
         {led[31], led[14]},
         {led[32], led[31], led[20], led[15], led[11]},
         {led[32], led[31], led[20], led[14], led[11]},
         {led[25], led[31], led[20], led[14]},
         {led[32], led[25], led[20], led[14], led[11]},
         {led[32], led[25], led[20], led[14], led[15], led[11]},
         {led[32], led[31], led[14]},
         {led[32], led[31], led[25], led[20], led[14], led[15], led[11]},
         {led[32], led[31], led[25], led[20], led[14], led[11]}
        },
        {// digit 2
         {led[30], led[24], led[29], led[13], led[8], led[10]},
         {led[29], led[8]},
         {led[30], led[29], led[19], led[13], led[10]},
         {led[30], led[29], led[19], led[8], led[10]},
         {led[24], led[29], led[19], led[8]},
         {led[30], led[24], led[19], led[8], led[10]},
         {led[30], led[24], led[19], led[13], led[8], led[10]},
         {led[30], led[29], led[8]},
         {led[30], led[29], led[24], led[19], led[13], led[8], led[10]},
         {led[30], led[29], led[24], led[19], led[8], led[10]}
		}
	};

	for (;;) {
		
		xEventGroupWaitBits(TASK_REG, TASK_DISPLAY_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);

		while (xEventGroupGetBits(TASK_REG) & TASK_DISPLAY_FLAG) {
			
			if (xQueueReceive(queue_display, &pkt, 0) == pdTRUE) {
				
                parse_speed_data(pkt.speed_data, &hundreds, &tens, &ones);			 
                if (pkt.engLoad_data > 100) pkt.engLoad_data = 100; 
                map_engLoad(pkt.engLoad_data, &engLoadBarCount);
                have_value = true;

                ESP_LOGI(TAG_LED, ">> Info: Speed: %d Engine_Load: %d", pkt.speed_data, pkt.engLoad_data);
            }

            for (int i = 0; i < 30; i++) {
                for (int j = 0; j < engLoadBarCount; j++) {
                    ;
                }
            }

            if (have_value) {

                bool leading_zero = true;                
                if (hundreds != 0) {
                    displayNumber(digits_numbers_leds, DIGIT_0, hundreds, leds_per_number[hundreds]);
                    leading_zero = false;
                }
                if (tens != 0 || !leading_zero) {
                    displayNumber(digits_numbers_leds, DIGIT_1, tens, leds_per_number[tens]);
                    leading_zero = false;
                }
                displayNumber(digits_numbers_leds, DIGIT_2, ones, leds_per_number[ones]);


                // + 1 because array begins at 0 and led count is one more. Led 7 is index 6. 
                displayEngLoad(engLoad_bar, engLoadBarCount, engLoadBarCount + 1);
            }
            vTaskDelay(pdMS_TO_TICKS(10));
        } 			
	}
}



void vTask_dummy_idle_mode(void *pvParameters) {
    ESP_LOGI(TAG_IDLE_MODE, "Idle mode started for energy saving");

    for (;;) {
        // Wait for idle mode flag
        xEventGroupWaitBits(TASK_REG, TASK_DUMMY_IDLE_MODE_FLAG, pdFALSE, pdFALSE, portMAX_DELAY);

        while (xEventGroupGetBits(TASK_REG) & TASK_DUMMY_IDLE_MODE_FLAG) {
            ESP_LOGW(TAG_IDLE_MODE, "INFO: Entering idle mode. (%d Seconds)", COOLDOWN_TIME_MS / 1000);

            long long start_time = esp_timer_get_time() / 1000;
            while ((esp_timer_get_time() / 1000) - start_time < COOLDOWN_TIME_MS) {  // COOLDOWN_TIME_MS in milliseconds
                 ESP_LOGI(TAG_IDLE_MODE, ">> Info: Idle mode active, waiting for wake-up signal. Time left: %lld milliseconds",
                    COOLDOWN_TIME_MS - ((esp_timer_get_time() / 1000) - start_time));
                vTaskDelay(pdMS_TO_TICKS(5500));  
            }

            ESP_LOGW(TAG_IDLE_MODE, "Warning: Woke-Up! Starting ESP-NOW and receive. Checking for signals");

            esp_now_start();
            vTaskDelay(pdMS_TO_TICKS(100));

            // Start receive task
            xEventGroupSetBits(TASK_REG, TASK_ESP_NOW_RECEIVE_FLAG);

            // Wait for connection flag for 5 seconds
            ESP_LOGW(TAG_IDLE_MODE, "INFO: Waiting for ESP-NOW connection signal for %d Seconds!", ESP_NOW_ACTIVE_TIME_MS / 1000);
            EventBits_t bits = xEventGroupWaitBits(STATUS_REG, STATUS_ESP_NOW_CONNECTION_FLAG, pdTRUE, pdTRUE, pdMS_TO_TICKS(ESP_NOW_ACTIVE_TIME_MS));
            
            if (bits & STATUS_ESP_NOW_CONNECTION_FLAG) {
                ESP_LOGI(TAG_IDLE_MODE, "Valid signal detected, exiting idle");
                xEventGroupClearBits(TASK_REG, TASK_DUMMY_IDLE_MODE_FLAG);  // Clear flag to exit
                vTaskDelay(pdMS_TO_TICKS(100));  
                break;

            } else {

                ESP_LOGW(TAG_IDLE_MODE, "WARNING: No signal. Stopping ESP-NOW and Receive. Starting over!");        
                xEventGroupClearBits(TASK_REG, TASK_ESP_NOW_RECEIVE_FLAG);   

                if (esp_now_stop() != ESP_OK) ESP_LOGE(TAG_IDLE_MODE, "Failed to stop ESP-NOW");
            }
        }
    }
}	

// modem sleep mode denen olaya bi bak. https://github.com/espressif/esp-idf/tree/v6.0/examples/wifi/power_save
void app_main(void) {

	TASK_REG = xEventGroupCreate();
	STATUS_REG = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	queue_esp_now_recv = xQueueCreate(10, sizeof(esp_now_data_packet_buff_t));
	queue_display = xQueueCreate(1, sizeof(esp_now_data_packet_buff_t));
 
	make_all_hiz();

	xTaskCreate(vTask_esp_now_receive, "ESP-NOW Receive", 8192, NULL, 2, NULL); 	
    xTaskCreate(vTask_dummy_idle_mode, "Dummy Idle Mode", 4096, NULL, 1, NULL);	
	xTaskCreate(vTask_display, "Display Task", 8192, NULL, 2, NULL);

	while (true) { 
		esp_err_t err = esp_now_start();
		vTaskDelay(pdMS_TO_TICKS(100));
		if (err == ESP_OK) {
			break;  // exit loop on success
		} else {
			vTaskDelay(pdMS_TO_TICKS(300));
		}
	}

	xEventGroupSetBits(TASK_REG, TASK_ESP_NOW_RECEIVE_FLAG);
    EventBits_t bits = xEventGroupWaitBits(STATUS_REG, STATUS_ESP_NOW_CONNECTION_FLAG, pdFALSE, pdFALSE, pdMS_TO_TICKS(3000));

    if (bits & STATUS_ESP_NOW_CONNECTION_FLAG) {
        ESP_LOGW(TAG_MAIN, "ESP-NOW connection detected!");
    } else {
        ESP_LOGW(TAG_MAIN, "ESP-NOW connection timeout after 3 seconds");
		xEventGroupSetBits(TASK_REG, TASK_DUMMY_IDLE_MODE_FLAG);
        xEventGroupClearBits(TASK_REG, TASK_ESP_NOW_RECEIVE_FLAG);
		esp_now_stop(); 
    }
}




    






