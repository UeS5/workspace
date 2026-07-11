#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "BAT";

// Voltage divider: R1=75k (bat->pin), R2=100k (pin->GND)
// Vbat = Vadc * (R1 + R2) / R2 = Vadc * 1.75
#define R1_KOHM     75.0f
#define R2_KOHM     100.0f
#define VDIV_MULT   ((R1_KOHM + R2_KOHM) / R2_KOHM)  // 1.75

#define ADC_UNIT    ADC_UNIT_1
#define ADC_CHANNEL ADC_CHANNEL_0   // GPIO0
#define ADC_ATTEN   ADC_ATTEN_DB_12 // 0~3.1V arasi

void app_main(void)
{
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg));

    // Kalibrasyon
    adc_cali_handle_t cali_handle = NULL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT,
        .chan     = ADC_CHANNEL,
        .atten    = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle) == ESP_OK) {
        calibrated = true;
        ESP_LOGI(TAG, "Kalibrasyon: curve fitting aktif");
    }
#endif

    if (!calibrated) {
        ESP_LOGW(TAG, "Kalibrasyon yok, olcumler kabaydir");
    }

    while (1) {
        int raw = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw));

        int adc_mv = 0;
        if (calibrated) {
            adc_cali_raw_to_voltage(cali_handle, raw, &adc_mv);
        } else {
            adc_mv = (raw * 3300) / 4095;
        }

        float bat_mv = adc_mv * VDIV_MULT;

        ESP_LOGI(TAG, "raw=%4d | adc=%4d mV | batarya=%.0f mV (%.2f V)",
                 raw, adc_mv, bat_mv, bat_mv / 1000.0f);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
