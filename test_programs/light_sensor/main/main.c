#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "light_sensor";

/* IO3 -> ADC1_CHANNEL_3 */
#define LIGHT_ADC_CHANNEL   ADC_CHANNEL_3
#define LIGHT_ADC_ATTEN     ADC_ATTEN_DB_12   /* 0-3.1V range */
#define SAMPLE_COUNT        16                 /* örnekleme sayısı (ortalama için) */
#define READ_INTERVAL_MS    500

/* Ham ADC değerini 0-100 arasında lux yüzdesine çevirir.
 * Pull-down devresi: ışık arttıkça transistör daha fazla iletir,
 * IO3 gerilimi yükselir → ADC değeri artar. */
static int adc_to_light_percent(int raw)
{
    /* ESP32-C3 ADC 12-bit -> 0..4095 */
    int percent = (raw * 100) / 4095;
    return percent;
}

void app_main(void)
{
    /* --- ADC Oneshot birim başlatma --- */
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_cfg, &adc_handle));

    /* --- Kanal yapılandırması --- */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,  /* 12-bit */
        .atten    = LIGHT_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, LIGHT_ADC_CHANNEL, &chan_cfg));

    /* --- Kalibrasyon (varsa) --- */
    adc_cali_handle_t cali_handle = NULL;
    bool do_cali = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT_1,
        .chan     = LIGHT_ADC_CHANNEL,
        .atten   = LIGHT_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle) == ESP_OK) {
        do_cali = true;
        ESP_LOGI(TAG, "Curve fitting kalibrasyon aktif");
    }
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT_1,
        .atten   = LIGHT_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_line_fitting(&cali_cfg, &cali_handle) == ESP_OK) {
        do_cali = true;
        ESP_LOGI(TAG, "Line fitting kalibrasyon aktif");
    }
#endif

    if (!do_cali) {
        ESP_LOGW(TAG, "Kalibrasyon desteklenmiyor, ham deger kullanilacak");
    }

    ESP_LOGI(TAG, "Isik sensoru baslatildi (IO3 / ADC1_CH3)");

    while (1) {
        /* Birden fazla örnek al, ortalamasını hesapla */
        int32_t sum = 0;
        for (int i = 0; i < SAMPLE_COUNT; i++) {
            int raw;
            ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, LIGHT_ADC_CHANNEL, &raw));
            sum += raw;
        }
        int raw_avg = (int)(sum / SAMPLE_COUNT);

        int light_pct = adc_to_light_percent(raw_avg);

        if (do_cali) {
            int voltage_mv;
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw_avg, &voltage_mv));
            ESP_LOGI(TAG, "Ham: %4d | Gerilim: %4d mV | Isik: %3d%%",
                     raw_avg, voltage_mv, light_pct);
        } else {
            ESP_LOGI(TAG, "Ham: %4d | Isik: %3d%%", raw_avg, light_pct);
        }

        vTaskDelay(pdMS_TO_TICKS(READ_INTERVAL_MS));
    }

    /* Temizlik (buraya normalde ulaşılmaz) */
    adc_oneshot_del_unit(adc_handle);
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (do_cali) adc_cali_delete_scheme_curve_fitting(cali_handle);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (do_cali) adc_cali_delete_scheme_line_fitting(cali_handle);
#endif
}
