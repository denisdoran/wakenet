#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "model_path.h"

#include "boa_mic.h"
//#include "driver/gpio.h"

#define MIC_BCLK_PIN  42
#define MIC_WS_PIN     2
#define MIC_SD_PIN    41

void app_main(void)
{
    // --- Load WakeNet model list ---
    srmodel_list_t *models = esp_srmodel_init("model");
    if (!models) {
        printf("ERROR: esp_srmodel_init() failed\n");
        return;
    }

    char *model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, "hiesp");
    if (!model_name) {
        printf("ERROR: No WakeNet model found matching 'hiesp'\n");
        return;
    }
    printf("Using WakeNet model: %s\n", model_name);

    esp_wn_iface_t *wakenet =
        (esp_wn_iface_t *)esp_wn_handle_from_name(model_name);
    if (!wakenet) {
        printf("ERROR: esp_wn_handle_from_name() failed\n");
        return;
    }

    model_iface_data_t *model_data =
        wakenet->create(model_name, DET_MODE_95);
    if (!model_data) {
        printf("ERROR: wakenet->create() failed\n");
        return;
    }

    int model_rate    = wakenet->get_samp_rate(model_data);
    int chunk_samples = wakenet->get_samp_chunksize(model_data);

    printf("WakeNet info:\n");
    printf("  model_name     = %s\n", model_name);
    printf("  sample_rate    = %d Hz\n", model_rate);
    printf("  chunk_samples  = %d\n", chunk_samples);

    // --- Allocate audio buffer ---
    int16_t *buffer = malloc(chunk_samples * sizeof(int16_t));
    if (!buffer) {
        printf("Failed to allocate audio buffer\n");
        return;
    }

    // --- Initialize microphone ---
    boa_mic_init(MIC_BCLK_PIN, MIC_WS_PIN, MIC_SD_PIN, model_rate);
    printf("Mic initialized at %d Hz\n", model_rate);

    // --- Initialize onboard RGB LED (mic level indicator) ---
    boa_mic_led_init();

    // --- Initialize external LED on GPIO18 ---
    boa_mic_gpio_init(18);


    int cooldown = 0;
    while (1) {

        // --- Read audio frame ---
        if (!boa_mic_read(buffer, chunk_samples)) {
            continue;
        }

        // --- Compute RMS for LED indicator ---
        int rms = boa_mic_rms(buffer, chunk_samples);
        boa_mic_led_set_level(rms);

        // --- WakeNet detection ---
        wakenet_state_t state = wakenet->detect(model_data, buffer);

        if (cooldown > 0) {
            cooldown--;
        }

    if (state == WAKENET_DETECTED && cooldown == 0) {
            // External LED flash
            boa_mic_pulse_gpio(18);   // clean, simple, reusable

            //  beep
            boa_mic_beep();

            // report to terminal
            printf("🎉 Detected Wake Word: \"Hi, ESP\" 🎉\n");
            cooldown = 30;
        }
    }

    // Cleanup (never reached)
    wakenet->destroy(model_data);
    free(buffer);
}
