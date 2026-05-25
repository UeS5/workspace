#ifndef DISPLAY_H

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
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

#define STATUS_ESP_NOW_FLAG				(1 << 0)
#define STATUS_ESP_NOW_CONNECTION_FLAG	(1 << 1)

#define TASK_ESP_NOW_RECEIVE_FLAG		(1 << 0)
#define TASK_DUMMY_IDLE_MODE_FLAG 		(1 << 1)
#define TASK_ESP_NOW_MONITOR_FLAG		(1 << 2)
#define TASK_DISPLAY_FLAG				(1 << 4)

#define BROADCAST_ADDR   { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }

#define COOLDOWN_TIME_MS 25000
#define ESP_NOW_ACTIVE_TIME_MS 5000
#define TIMEOTUT_CHECK_MS 10000

#define PIN_COUNT 6

#define P1 GPIO_NUM_1
#define P4 GPIO_NUM_4
#define P5 GPIO_NUM_5
#define P6 GPIO_NUM_6
#define P7 GPIO_NUM_7
#define P10 GPIO_NUM_10

#define DIGIT_COUNT 3
#define NUMBER_COUNT 10
#define MAX_LED_PER_DIGIT 7

#define BAR_LED_COUNT 7

#define DIGIT_0 0
#define DIGIT_1 1
#define DIGIT_2 2
	

#define LED_ON_TIME_MS 5
#define LED_OFF_TIME_MS 2
#define LED_ON_TIME 700
#define LED_OFF_TIME 200

// Each number has a different led count
#define NR0_LED_COUNT 6
#define NR1_LED_COUNT 2
#define NR2_LED_COUNT 5
#define NR3_LED_COUNT 5
#define NR4_LED_COUNT 4
#define NR5_LED_COUNT 5
#define NR6_LED_COUNT 6
#define NR7_LED_COUNT 3
#define NR8_LED_COUNT 7
#define NR9_LED_COUNT 6


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

typedef struct {
    gpio_num_t high;
    gpio_num_t low;
} led_t;

void make_all_hiz(void);
void pulse_led(led_t led);
void display(led_t digits_numbers_leds[DIGIT_COUNT][NUMBER_COUNT][MAX_LED_PER_DIGIT], uint8_t digit, uint8_t number, uint8_t led_count);
void parse_speed_data(uint8_t speed, uint8_t *hundreds, uint8_t *tens, uint8_t *ones);

#endif