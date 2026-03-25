#include "boa_mic.h"
#include "driver/ledc.h"
#include "driver/i2s_std.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

#include "led_strip.h"   // WS2812 driver (v1 API)

static const char *TAG = "BOA_MIC";
static i2s_chan_handle_t rx_chan;
static led_strip_handle_t boa_led = NULL;   // onboard RGB LED handle


// ============================================================
//  MIC INITIALIZATION
// ============================================================
void boa_mic_init(int bclk_pin, int ws_pin, int sd_pin, int sample_rate)
{
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_chan));

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),

        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT,
            .slot_mode      = I2S_SLOT_MODE_MONO,
            .slot_mask      = I2S_STD_SLOT_LEFT,   // CRITICAL FIX
            .ws_width       = 32,
            .ws_pol         = false,
            .bit_shift      = true,
            .left_align     = true,
            .big_endian     = false,
            .bit_order_lsb  = false,
        },

        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = bclk_pin,
            .ws   = ws_pin,
            .dout = I2S_GPIO_UNUSED,
            .din  = sd_pin,
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));

    ESP_LOGI(TAG, "ICS-43434 mic initialized at %d Hz", sample_rate);
}


// ============================================================
//  MIC READ
// ============================================================
bool boa_mic_read(int16_t *buffer, size_t samples)
{
    size_t bytes_read = 0;
    const size_t bytes_needed = samples * sizeof(int32_t);

    static int32_t raw32[2048];

    esp_err_t ret =
        i2s_channel_read(rx_chan, raw32, bytes_needed, &bytes_read, portMAX_DELAY);

    if (ret != ESP_OK || bytes_read != bytes_needed) {
        return false;
    }

    // Convert 32‑bit left‑justified to 16‑bit PCM
    for (size_t i = 0; i < samples; i++) {
        buffer[i] = (int16_t)(raw32[i] >> 12);
    }

    return true;
}


// ============================================================
//  RMS CALCULATION
// ============================================================
int boa_mic_rms(const int16_t *buffer, size_t samples)
{
    int64_t sum = 0;
    for (size_t i = 0; i < samples; i++) {
        sum += (int32_t)buffer[i] * buffer[i];
    }
    return (int)sqrtf((float)sum / samples);
}


// ============================================================
//  ONBOARD RGB LED (WS2812) INITIALIZATION — ESP‑IDF 5.5.3
// ============================================================
void boa_mic_led_init(void)
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = 48,   // onboard RGB LED
        .max_leds = 1,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10 MHz
        .mem_block_symbols = 64,
        .flags.with_dma = false,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &boa_led));
    led_strip_clear(boa_led);
}


// ============================================================
//  MIC LEVEL → LED BRIGHTNESS
// ============================================================
void boa_mic_led_set_level(int rms)
{
    if (!boa_led) return;

    // New noise floor: ignore everything below ~6000
    if (rms < 6000) {
        led_strip_set_pixel(boa_led, 0, 0, 0, 0);
        led_strip_refresh(boa_led);
        return;
    }

    // Clamp
    if (rms > 20000) rms = 20000;

    // Map 6000–20000 → 0–255
    int brightness = ((rms - 6000) * 255) / 14000;

    uint8_t r = brightness / 2;
    uint8_t g = brightness / 2;
    uint8_t b = brightness;

    led_strip_set_pixel(boa_led, 0, r, g, b);
    led_strip_refresh(boa_led);
}




// ============================================================
//  COMMUNICATOR CHIRP BEEP
// ============================================================
void boa_mic_beep(void)
{
    const int beep_pin = 6;

    ledc_timer_config_t timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .freq_hz          = 1200,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .gpio_num       = beep_pin,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 800,   // louder
        .hpoint         = 0
    };
    ledc_channel_config(&channel);

    // TNG‑style chirp (louder frequencies)
    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, 2700);
    vTaskDelay(pdMS_TO_TICKS(40));

    ledc_set_freq(LEDC_LOW_SPEED_MODE, LEDC_TIMER_0, 4000);
    vTaskDelay(pdMS_TO_TICKS(40));

    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    gpio_reset_pin(beep_pin);
}



void boa_mic_gpio_init(int pin)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);

    gpio_set_level(pin, 0);
}

void boa_mic_pulse_gpio(int pin)
{
    gpio_set_level(pin, 1);
    vTaskDelay(pdMS_TO_TICKS(150));
    gpio_set_level(pin, 0);
}

